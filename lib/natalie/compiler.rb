require 'tempfile'
require 'sexp_processor'
require_relative './compiler/pass1'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'

module Natalie
  class Compiler
    ROOT_DIR = File.expand_path('../../', __dir__)
    SRC_PATH = File.join(ROOT_DIR, 'src')
    INC_PATHS = [
      File.join(ROOT_DIR, 'include'),
      File.join(ROOT_DIR, 'ext/bdwgc/include'),
      File.join(ROOT_DIR, 'ext/onigmo'),
      File.join(ROOT_DIR, 'ext/hashmap/include'),
    ]
    GC_LIB_PATHS = ENV['NAT_CFLAGS'] =~ /NAT_GC_DISABLE/ ? [] : [
      File.join(ROOT_DIR, 'ext/bdwgc/.libs/libgc.a'),
      File.join(ROOT_DIR, 'ext/bdwgc/.libs/libgccpp.a'),
    ]
    LIB_PATHS = GC_LIB_PATHS + [
      File.join(ROOT_DIR, 'ext/hashmap/build/libhashmap.a'),
      File.join(ROOT_DIR, 'ext/onigmo/.libs/libonigmo.a'),
    ]
    OBJ_PATH = File.expand_path('../../obj', __dir__)

    MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main.cpp'))
    OBJ_TEMPLATE = <<-EOF
      #{MAIN_TEMPLATE.split(/\/\* end of front matter \*\//).first}

      /*TOP*/

      Value *obj_%{name}(Env *env, Value *self) {
        /*BODY*/
        return NAT_NIL;
      }
    EOF

    class CompileError < StandardError; end

    def initialize(ast, path, options = {})
      @ast = ast
      @var_num = 0
      @path = path
      @options = options
      @required = {}
    end

    attr_accessor :ast, :compile_to_object_file, :repl, :out_path, :context, :vars, :options, :c_path

    attr_writer :load_path

    def compile
      check_build
      write_file
      compile_c_to_binary
    end

    def compile_c_to_binary
      cmd = compiler_command
      out = `#{cmd} 2>&1`
      File.unlink(@c_path) unless debug || build == 'coverage'
      $stderr.puts out if out.strip != ''
      raise CompileError.new('There was an error compiling.') if $? != 0
    end

    def check_build
      if Dir[File.join(OBJ_PATH, '*.o')].none?
        puts 'please run: make build'
        exit 1
      end
    end

    def write_file
      c = to_c
      temp_c = Tempfile.create('natalie.cpp')
      temp_c.write(c)
      temp_c.close
      @c_path = temp_c.path
    end

    def build_context
      {
        var_prefix: var_prefix,
        var_num: 0,
        template: template,
        repl: repl,
        vars: vars || {}
      }
    end

    def transform(ast)
      @context = build_context

      ast = Pass1.new(@context).go(ast)
      if debug == 'p1'
        pp ast
        exit
      end

      ast = Pass2.new(@context).go(ast)
      if debug == 'p2'
        pp ast
        exit
      end

      ast = Pass3.new(@context).go(ast)
      if debug == 'p3'
        pp ast
        exit
      end

      Pass4.new(@context).go(ast)
    end

    def to_c
      @ast = expand_macros(@ast, @path)
      reindent(transform(@ast))
    end

    def load_path
      Array(@load_path)
    end

    def debug
      options[:debug]
    end

    def build
      options[:build]
    end

    def inc_paths
      INC_PATHS.map { |path| "-I #{path}" }.join(' ')
    end

    def compiler_command
      if compile_to_object_file
        "#{cc} #{build_flags} #{extra_cflags} #{inc_paths} -fPIC -x c++ -std=c++17 -c #{@c_path} -o #{out_path}"
      else
        libs = '-lm'
        "#{cc} #{build_flags} #{extra_cflags} #{shared? ? '-fPIC -shared' : ''} #{inc_paths} -o #{out_path} #{OBJ_PATH}/*.o #{OBJ_PATH}/nat/*.o #{LIB_PATHS.join(' ')} -x c++ -std=c++17 #{@c_path || 'code.cpp'} #{libs}"
      end
    end

    private

    def cc
      ENV['CXX'] || 'c++'
    end

    def shared?
      !!repl
    end

    RELEASE_FLAGS = '-O1'
    DEBUG_FLAGS = '-g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unknown-warning-option'
    COVERAGE_FLAGS = '-fprofile-arcs -ftest-coverage'

    def build_flags
      case build
      when 'release'
        RELEASE_FLAGS
      when 'debug', nil
        DEBUG_FLAGS
      when 'coverage'
        DEBUG_FLAGS + ' ' + COVERAGE_FLAGS
      else
        raise "unknown build mode: #{build.inspect}"
      end
    end

    def extra_cflags
      ENV['NAT_CFLAGS']
    end

    def var_prefix
      if compile_to_object_file
        "#{obj_name}_"
      else
        ''
      end
    end

    def obj_name
      out_path.sub(/.*obj\//, '').sub(/\.o$/, '').tr('/', '_').sub(/^nat_/, '')
    end

    def template
      if compile_to_object_file
        OBJ_TEMPLATE % { name: obj_name }
      else
        MAIN_TEMPLATE
      end
    end

    def expand_macros(ast, path)
      (0...(ast.size)).reverse_each do |i|
        node = ast[i]
        if macro?(node)
          ast[i,1] = run_macro(node, path)
        end
      end
      ast
    end

    def macro?(node)
      return false unless node[0..1] == s(:call, nil)
      %i[require require_relative load].include?(node[2])
    end

    def run_macro(expr, path)
      (_, _, macro, *args) = expr
      send("macro_#{macro}", *args, path)
    end

    REQUIRE_EXTENSIONS = %w[nat rb]

    def macro_require(node, current_path)
      name = node[1]
      if File.extname(name).empty?
        REQUIRE_EXTENSIONS.each do |extension|
          path = "#{name}.#{extension}"
          next unless full_path = find_full_path(path, base: Dir.pwd, search: true)
          return load_file(full_path, require_once: true)
        end
      elsif (full_path = find_full_path(name, base: Dir.pwd, search: true))
        return load_file(full_path, require_once: true)
      end
      raise LoadError, "cannot load such file #{node.file}##{node.line}-- #{name}.{#{REQUIRE_EXTENSIONS.join(',')}}"
    end

    def macro_require_relative(node, current_path)
      name = node[1]
      if File.extname(name).empty?
        REQUIRE_EXTENSIONS.each do |extension|
          path = "#{name}.#{extension}"
          next unless full_path = find_full_path(path, base: File.dirname(current_path), search: false)
          return load_file(full_path, require_once: true)
        end
      elsif (full_path = find_full_path(name, base: File.dirname(current_path), search: false))
        return load_file(full_path, require_once: true)
      end
      raise LoadError, "cannot load such file at #{node.file}##{node.line} -- #{name}.{#{REQUIRE_EXTENSIONS.join(',')}}"
    end

    def macro_load(node, _)
      path = node.last
      full_path = find_full_path(path, base: Dir.pwd, search: true)
      if full_path
        load_file(full_path, require_once: false)
      else
        raise LoadError, "cannot load such file -- #{path}"
      end
    end

    def load_file(path, require_once:)
      return s(:block) if require_once && @required[path]
      @required[path] = true
      code = File.read(path)
      file_ast = Natalie::Parser.new(code, path).ast
      expand_macros(file_ast, path)
    end

    def find_full_path(path, base:, search:)
      if path.start_with?(File::SEPARATOR)
        path if File.exist?(path)
      elsif path.start_with?('.' + File::SEPARATOR)
        path = File.expand_path(path, base)
        path if File.exist?(path)
      elsif search
        find_file_in_load_path(path)
      else
        path = File.expand_path(path, base)
        path if File.exist?(path)
      end
    end

    def find_file_in_load_path(path)
      load_path.map { |d| File.join(d, path) }.detect { |p| File.exist?(p) }
    end

    def reindent(code)
      out = []
      indent = 0
      code.split("\n").each do |line|
        indent -= 4 if line =~ /^\s*\}/
        indent = [0, indent].max
        out << line.sub(/^\s*/, ' ' * indent)
        indent += 4 if line.end_with?('{')
      end
      out.join("\n")
    end
  end
end
