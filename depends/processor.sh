#!/bin/bash
exec 3< cpfile
while read <&3
do echo $REPLY | cpio -pvdm cleansed
done
exec 3>&- 
