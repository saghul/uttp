{
  'targets': [
    {
      'dependencies': [
        'deps/libuv/uv.gyp:libuv',
        'deps/http-parser/http_parser.gyp:http_parser',
      ],
      'target_name': 'uttp',
      'type': 'executable',
      'sources': [
        'src/defs.h',
        'src/log.h',
        'src/main.c',
        'src/server.c',
        'src/server.h',
        'src/worker.c',
        'src/worker.h',
      ]
    }
  ]
}
