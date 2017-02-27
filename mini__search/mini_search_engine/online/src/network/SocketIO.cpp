 ///
 /// @file    SocketIO.cc
 /// @author  lemon(haohb13@gmail.com)
 /// @date    2015-11-05 16:14:19
 ///

#include "SocketIO.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>


namespace wd
{

SocketIO::SocketIO(int sockfd)
	: sockfd_(sockfd)
{}


size_t SocketIO::readn(char * buf, size_t count)
{
	size_t nleft = count;
	char * pbuf = buf;
	while(nleft > 0)
	{
		int nread = ::read(sockfd_, pbuf, nleft);
		if(-1 == nread)
		{
			if(errno == EINTR)//忽略掉中断信号
				continue;
			return EXIT_FAILURE;
		}
		else if(0 == nread) //EOF
		{
			break;
		}
		pbuf += nread;
		nleft -= nread;
	}
	return (count - nleft);
}


size_t SocketIO::writen(const char * buf, size_t count)
{
	size_t nleft = count;
	const char * pbuf = buf;
	while(nleft > 0)
	{
		int nwrite = ::write(sockfd_, pbuf, nleft);
		if(nwrite == -1)
		{
			if(errno == EINTR)
				continue;
			return EXIT_FAILURE;
		}
		nleft -= nwrite;
		pbuf += nwrite;
	}
	return (count - nleft);
}

size_t SocketIO::recv_peek(char * buf, size_t count)
{
	int nread;
	do
	{
		nread = ::recv(sockfd_, buf, count, MSG_PEEK);//recv即使是MSG_PEEK也不一定可以一次预览count个字符
	}while(nread == -1 && errno == EINTR);
	return nread;
}

size_t SocketIO::readline(char * buf, size_t maxlen)
{
	size_t nleft = maxlen - 1;
	char * pbuf = buf;
	size_t total = 0;
	while(nleft > 0)//循环预览，直到所有的字符都预览完（解决不能一次读入太多字符的问题）
	{
		int nread = recv_peek(pbuf, nleft);//pbuf取到recv到的数据
		if(nread <= 0)
			return nread;

		//检查'\n'
		for(size_t idx = 0; idx != nread; ++idx)
		{
			if(pbuf[idx] == '\n')
			{
				size_t nsize = idx + 1;
				if(readn(pbuf, nsize) != nsize)
					return EXIT_FAILURE;
				pbuf += nsize;
				total += nsize;
				*pbuf = 0;
				return total;
			}
		}
		if(readn(pbuf, nread) != nread)
			return EXIT_FAILURE;
		pbuf += nread;
		nleft -= nread;
		total += nread;
	}
	*pbuf = 0;//将最后一个字符置为0，字符串结束标志
	return maxlen - 1;
}

}// end of namespace wd
