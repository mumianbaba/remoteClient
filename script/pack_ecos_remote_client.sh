#!/bin/bash


basePath="release"
baseName="ecos-remote-client"

line1=`cat ./${basePath}/version.txt | grep "version"`
version=`echo ${line1} | awk -F " " '{print $2}'`

name="${baseName}-v${version}.tar.gz"
time=$(date "+%Y-%m-%d %H:%M:%S")

echo "${line1}" > ${basePath}/version.txt
echo "date: ${time}" >> ${basePath}/version.txt
md5sum ${basePath}/term >> ${basePath}/version.txt

tar -czvf ${name} "${basePath}"
