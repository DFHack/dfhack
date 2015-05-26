#!/bin/bash

BUILD_DIR=`pwd`

echo "Fixing library dependencies in $BUILD_DIR/library"

install_name_tool -change $BUILD_DIR/library/libdfhack.1.0.0.dylib @executable_path/hack/libdfhack.1.0.0.dylib library/libdfhack.1.0.0.dylib
install_name_tool -change $BUILD_DIR/library/libdfhack-client.dylib @executable_path/hack/libdfhack-client.dylib library/libdfhack-client.dylib
install_name_tool -change $BUILD_DIR/library/libdfhack-client.dylib @executable_path/hack/libdfhack-client.dylib library/dfhack-run
install_name_tool -change $BUILD_DIR/depends/protobuf/libprotobuf-lite.dylib @executable_path/hack/libprotobuf-lite.dylib library/libdfhack.1.0.0.dylib
install_name_tool -change $BUILD_DIR/depends/protobuf/libprotobuf-lite.dylib @executable_path/hack/libprotobuf-lite.dylib library/libdfhack-client.dylib
install_name_tool -change $BUILD_DIR/depends/protobuf/libprotobuf-lite.dylib @executable_path/hack/libprotobuf-lite.dylib library/dfhack-run
install_name_tool -change $BUILD_DIR/depends/lua/liblua.dylib @executable_path/hack/liblua.dylib library/libdfhack.1.0.0.dylib
install_name_tool -change @executable_path/../Frameworks/SDL.framework/Versions/A/SDL @executable_path/libs/SDL.framework/Versions/A/SDL library/libdfhack.1.0.0.dylib
install_name_tool -change /usr/local/lib/libstdc++.6.dylib @executable_path/libs/libstdc++.6.dylib library/libdfhack.1.0.0.dylib
install_name_tool -change /opt/local/lib/i386/libstdc++.6.dylib @executable_path/libs/libstdc++.6.dylib library/libdfhack.1.0.0.dylib
install_name_tool -change /opt/local/lib/i386/libstdc++.6.dylib @executable_path/libs/libstdc++.6.dylib library/libdfhack-client.dylib
install_name_tool -change /opt/local/lib/i386/libstdc++.6.dylib @executable_path/libs/libstdc++.6.dylib library/dfhack-run
install_name_tool -change /opt/local/lib/i386/libgcc_s.1.dylib @executable_path/libs/libgcc_s.1.dylib library/libdfhack.1.0.0.dylib
install_name_tool -change /opt/local/lib/i386/libgcc_s.1.dylib @executable_path/libs/libgcc_s.1.dylib library/libdfhack-client.dylib
install_name_tool -change /opt/local/lib/i386/libgcc_s.1.dylib @executable_path/libs/libgcc_s.1.dylib library/dfhack-run
