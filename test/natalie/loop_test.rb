require_relative '../spec_helper'

describe 'while' do
  it 'works' do
    r = []
    x = 10
    while x != 0
      r << x
      x = x - 1
    end
    r.should == [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  end
end

describe 'until' do
  it 'works' do
    r = []
    x = 10
    until x == 0
      r << x
      x = x - 1
    end
    r.should == [10, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  end
end

describe 'each' do
  it 'can set variables outside the block' do
    x = 0
    [1, 2, 3].each do |i|
      x = i
    end
    x.should == 3
  end
end
