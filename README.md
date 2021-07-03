# remoteClient
一、编译
  下载源码，直接make 即可，使用c代码编写，不依靠什么库。编译会得到一个可执行程序trem和一个压缩包ecos-remote-client-v0.01.tar.gz。这个程序是和remoteServer 程序配套使用的。
  
二、使用
运行./term 会有提示。
2.1 使用配置文件
./term MYID   这种用法，term 会读取当前目录的client.cfg的配置文件。
>root@workshop:/tmp/release# cat client.cfg 
>
>server_ip 172.16.3.95 
>
>server_port 81

2.2 不读取配置文件，使用输入参数
>./term IP PORT MYID TYPE
>
> MYID：可以是任何字符，但是长度不要太长。
> 
> TYPD：c代表着调试客户端，d代表着被调试子设备。
> 
>./term 192.168.99.117 81 MYID c

所以可以使用term 同时模拟调试客户端和调试设备。
