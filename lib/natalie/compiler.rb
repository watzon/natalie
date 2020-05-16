require 'tempfile'
require 'sexp_processor'
require_relative './compiler/pass1'
require_relative './compiler/pass2'
require_relative './compiler/pass3'
require_relative './compiler/pass4'

module Natalie
  class Compiler
    SRC_PATH = File.expand_path('../../src', __dir__)
    INC_PATHS = [
      File.expand_path('../../include', __dir__),
      File.expand_path('../../ext/onigmo', __dir__),
      File.expand_path('../../ext/hashmap/include', __dir__),
    ]
    LIBNATALIE_PATH = File.expand_path('../../build/libnatalie.a', __dir__)
    LIBHASHMAP_PATH = File.expand_path('../../ext/hashmap/build/libhashmap.a', __dir__)
    LIBONIGMO_PATH = File.expand_path('../../ext/onigmo/.libs/libonigmo.a', __dir__)
    LIBS = [
      LIBNATALIE_PATH,
      LIBHASHMAP_PATH,
      LIBONIGMO_PATH,
    ]

    MAIN_TEMPLATE = File.read(File.join(SRC_PATH, 'main.c'))
    OBJ_TEMPLATE = <<-EOF
      #{MAIN_TEMPLATE.split(/\/\* end of front matter \*\//).first}

      /*TOP*/

      NatObject *obj_%{name}(NatEnv *env, NatObject *self) {
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
    end

    attr_accessor :ast, :compile_to_object_file, :repl, :out_path, :context, :vars, :options, :c_path

    attr_writer :load_path

    def compile
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
      unless File.exist?(LIBNATALIE_PATH)
        puts 'please build natalie first :-)'
        exit 1
      end
    end

    def write_file
      c = to_c
      temp_c = Tempfile.create('natalie.c')
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
        "#{cc} #{build_flags} #{extra_cflags} #{inc_paths} -fPIC -x c -c #{@c_path} -o #{out_path}"
      else
        check_build
        "#{cc} #{build_flags} #{extra_cflags} #{shared? ? '-fPIC -shared' : ''} #{inc_paths} -o #{out_path} -x c #{@c_path || 'code.c'} -x none #{LIBS.join(' ')} -lm"
      end
    end

    private

    def cc
      ENV['CC'] || 'cc'
    end

    def shared?
      !!repl
    end

    RELEASE_FLAGS = '-O1'
    DEBUG_FLAGS = '-g -Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unknown-warning-option -D"NAT_GC_COLLECT_DEBUG=true"'
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
      out_path.sub(/.*obj\//, '').sub(/\.o$/, '').tr('/', '_')
    end

    def template
      if compile_to_object_file
        OBJ_TEMPLATE % { name: obj_name }
      elsif repl
        MAIN_TEMPLATE.sub(/env->global_env->gc_enabled = true;/, '')
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
      REQUIRE_EXTENSIONS.each do |extension|
        path = "#{name}.#{extension}"
        next unless full_path = find_full_path(path, base: Dir.pwd, search: true)
        return load_file(full_path)
      end
      raise LoadError, "cannot load such file -- #{name}.{#{REQUIRE_EXTENSIONS.join(',')}}"
    end

    def macro_require_relative(node, current_path)
      name = node[1]
      REQUIRE_EXTENSIONS.each do |extension|
        path = "#{name}.#{extension}"
        next unless full_path = find_full_path(path, base: File.dirname(current_path), search: false)
        return load_file(full_path)
      end
      raise LoadError, "cannot load such file -- #{name}.{#{REQUIRE_EXTENSIONS.join(',')}}"
    end

    def macro_load(node, _)
      path = node.last
      full_path = find_full_path(path, base: Dir.pwd, search: true)
      if full_path
        load_file(full_path)
      else
        raise LoadError, "cannot load such file -- #{path}"
      end
    end

    def load_file(path)
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
