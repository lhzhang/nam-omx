#!/bin/bash

# 递归改名，用find+rename, 仅仅rename不能递归修改
# 内容替换，用find+sed
# http://www.haogongju.net/art/497188

find ./nam_omx -name "*" -exec rename 's/sec/nam/' {} \;
find ./nam_omx -name "*" -exec rename 's/Sec/Nam/' {} \;
find ./nam_omx -name "*" -exec rename 's/SEC/NAM/' {} \;
