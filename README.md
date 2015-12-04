#Simple FCGI Process Manager

轻量级FCGI管理程序，可用于PHP或一般FCGI进程的管理。

* 可做为 spawn-fcgi 的替换，启动多个FCGI进程侦听同一端口
* 退出时保证所有子进程同时退出
* 可维护一定数量的FCGI进程，FCGI意外退出后自动启动新进程


## 使用方法

### Windows版本

	Usage: fpm path [-n number] [-i ip] [-p port]
	Manage FastCGI processes.
	
	-f, filename of the fcgi processor
	-n, number of processes to keep, default is 1
	-i, ip address to bind, default is 127.0.0.1
	-p, port to bind, default is 9000
	-h, output usage information and exit
	-v, output version information and exit

### Linux版本

	TODO
	

## 类似项目

*  Lighttpd [spawn-fcgi](https://github.com/lighttpd/spawn-fcgi)
*  XiaoXia  [xxfpm](https://github.com/78/xxfpm) 


