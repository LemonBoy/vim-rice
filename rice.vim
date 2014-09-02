" rice - A FFI for vim

function s:ReadRes(str)
    if a:str == ''
        return ''
    elseif a:str == 'v'
        return 'v'
    elseif a:str[0] == 'i'
        return str2nr(a:str[1:-2])
    else 
        let i = stridx(a:str, ':')
        if i < 0
            return ''
        endif
        return a:str[i+1:]
    endif
endfunction

function rice#RevI(ptr)
    let a = a:ptr[0] . a:ptr[1]
    let b = a:ptr[2] . a:ptr[3]
    let c = a:ptr[4] . a:ptr[5]
    let d = a:ptr[6] . a:ptr[7]
    return str2nr(d.c.b.a, 16)
endfunction

function rice#PackI(i)
    return printf('i%ie', a:i)
endfunction

function rice#PackS(str)
    return printf('%i:%s', strlen(a:str), a:str)
endfunction

function rice#ReadPtr(addr, size)
    return libcall('librice.so', 'rptr', rice#PackI(a:size).rice#PackI(a:addr))
endfunction

function rice#Bind(libpath, libs)
    let type_lut = {
    \ 'W' : 'a:%i',
    \ 'I' : 'rice#PackI(a:%i)',
    \ 'S' : 'rice#PackS(a:%i)',
    \ }

    for [k,v] in items(a:libs)
        " Functions names must start with a capital
        let fn = 'L'.k.v
        let tmp = join([a:libpath, k, v[0]], ';')

        " Build the n-parameter list thunk
        let p = []
        let n = strlen(v[1:])
        
        for i in range(1, n)
            let p = add(p, printf(type_lut[v[i]], i))
        endfor 

        " Build a lambda that wraps up the call
        exe 'function! ' . fn . ' (...)' . "\n"
        \.  'let i = [' . join(p, ',') ']' . "\n"
        \.  "let p = '" . tmp . ";' . len(a:000) . ';' . join(i, '')" . "\n"
        \.  "return s:ReadRes(libcall('librice.so', 'call', p))" . "\n"
        \.  'endfunction' . "\n"

        " Replace the definition
        let a:libs[k] = function(fn)
    endfor
    " Compact
    call garbagecollect()
endfunction 
