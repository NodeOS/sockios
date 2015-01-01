{
  "targets": [
    {
      "target_name": "binding",
      "sources": [ "src/sockios.cc" ],
      "include_dirs": [ "<!(node -e \"require('nan')\")" ]
    }
  ]
}
