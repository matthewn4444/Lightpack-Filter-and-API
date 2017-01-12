{
    "targets": [
        {
            "target_name": "lightpack",
            "include_dirs": [
                "../../include"
            ],
            "link_settings": {
                "libraries": [
                    "lightpack.lib",
                ],
                "library_dirs": [
                    "../../lib",
                ],
            },
            "sources": [ "src/lightpack.cc" ]
        }
    ]
}