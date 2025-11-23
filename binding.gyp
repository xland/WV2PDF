{
  "targets": [
    {
        "include_dirs": ["<!(node -p \"require('node-addon-api').include_dir\")",
                         "D:\\sdk\\cpp-ipc\\include"],
        "target_name": "native",
        "defines": [ 
            'NAPI_DISABLE_CPP_EXCEPTIONS',
            "NODE_ADDON_API_ENABLE_MAYBE",
            "UNICODE",
            "_UNICODE"
          ],
        "sources": [ 
            "src/main.cc",
            "src/Worker.h",
            "src/Worker.cc"
            ],
        "libraries": [
            "D:\\sdk\\cpp-ipc\\build\\lib\\Release\\ipc.lib"
        ],
    }
  ]
}