" The first char is the return type, it can be one of the following
" I = 32 bit int, used for pointers too
" V = void
" You can specify the function arguments (if any) after that, you can use the
" types above plus the 'W' which tells rice not to interpret the argument,
" useful for functions accepting different kinds of params (see
" curl_easy_setopt for an example on how to use it.

let curl = {
\ 'curl_easy_init'      : 'I',
\ 'curl_easy_setopt'    : 'VIIW',
\ 'curl_easy_perform'   : 'II',
\ 'curl_easy_cleanup'   : 'VI',
\ }

let libc = {
\ 'malloc'  : 'II',
\ 'memset'  : 'VIII',
\ 'free'    : 'VI'
\ }

let libxml = {
\ 'xmlReadFile'             : 'ISII',
\ 'xmlDocGetRootElement'    : 'II',
\ 'xmlFreeDoc'              : 'VI',
\ 'xmlCleanupParser'        : 'V',
\ }

call rice#Bind('libcurl.so', curl)
call rice#Bind('libc.so.6', libc)
call rice#Bind('libxml2.so', libxml)

" {{{
    let doc = libxml.xmlReadFile('test.xml', 0, 0)
    if !doc
        echo "Fail :("
    else 
        let root = libxml.xmlDocGetRootElement(doc)

        " Get the topmost 4 bytes only
        let name_ptr = rice#ReadPtr(root + 16, 60)[:8]

        let name = rice#ReadPtr(rice#RevI(name_ptr), -1)

        echo 'Root name : ' . name . "\n"

        call libxml.xmlFreeDoc(doc)
        call libxml.xmlCleanupParser()
    endif
" }}}

" {{{
    let ctx = curl.curl_easy_init()
    call curl.curl_easy_setopt(ctx, 10002, rice#PackS('http://example.com'))
    call curl.curl_easy_setopt(ctx, 52, rice#PackI('1'))
    let res = curl.curl_easy_perform(ctx)
    call curl.curl_easy_cleanup(ctx)
" }}}

" {{{
    let mem = libc.malloc(16)
    call libc.memset(mem+0, 0x40, 8)
    call libc.memset(mem+4, 0x80, 8)
    let test = rice#ReadPtr(mem, 8)
    echo printf("Dump of ptr %016X - %s\n", mem, test)
    call libc.free(mem)
" }}}
