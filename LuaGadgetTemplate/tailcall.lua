function foo1()
    return 'LuaHunt'
end

function foo2()
    return foo1()
end

print(foo2())