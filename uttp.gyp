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
        'src/main.c',
      ]
    }
  ]
}
