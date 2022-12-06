#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

typedef struct s_client
{
	int fd;
	int id;
}				t_client;

t_client g_client[1000000];
int sockfd, id, max_fd;
char msg[1000000], buf[1000000];
fd_set fd_rd, fd_wr, curr_fd;

void	exit_err(char *msg_err)
{
	close(sockfd);
	write(2, msg_err, strlen(msg_err));
	exit(1);
}

void	send_all(int client_id)
{
	for (int i = 0; i < id; i++)
	{
		if (client_id != g_client[i].id && FD_ISSET(g_client[i].fd, &fd_wr))
			if (send(g_client[i].fd, msg, strlen(msg), 0) < 0)
				exit_err("Fatal error\n");
	}
	bzero(msg, sizeof(msg));
}

void	add_client(void)
{
	int i = 0;
	while (g_client[i].fd > 0 && i < 1024)
		i++;
	struct sockaddr_in clientaddr;
	socklen_t len = sizeof(clientaddr);

	g_client[i].fd = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
	if (g_client[i].fd == -1)
		exit_err("Fatal error\n");
	g_client[i].id = id++;
	FD_SET(g_client[i].fd, &curr_fd);
	if (g_client[i].fd > max_fd)
		max_fd = g_client[i].fd;
	sprintf(msg, "server: client %d just arrived\n", g_client[i].id);
	send_all(g_client[i].id);
}

void	extract_msg(int client_id)
{
	int i = 0, j = 0;
	char tmp[100000] = {0};
	int len = strlen(buf);

	while (i < len)
	{
		tmp[j++] = buf[i];
		if (buf[i++] == '\n')
		{
			sprintf(msg + strlen(msg), "client %d: %s", client_id, tmp);
			bzero(tmp, sizeof(tmp));
			j = 0;
		}
	}
	send_all(client_id);
}

int	init_server(int port)
{
	struct sockaddr_in servaddr;

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // A remplacer par htonl()
	servaddr.sin_port = htons(port);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
		exit_err("Fatal error\n");
	if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
		exit_err("Fatal error\n");
	if (listen(sockfd, 128) < 0)
		exit_err("Fatal error\n");
	
	return (sockfd);
}

int main(int ac, char **av)
{
	if (ac != 2)
		exit_err("Fatal error\n");
	
	max_fd = sockfd = init_server(atoi(av[1]));

	FD_ZERO(&curr_fd);
	FD_SET(sockfd, &curr_fd);

	while (1)
	{
		fd_rd = fd_wr = curr_fd;
		if (select(max_fd + 1, &fd_rd, &fd_wr, NULL, NULL) < 0)
			exit_err("Fatal error\n");
		if (FD_ISSET(sockfd, &fd_rd))
			add_client();
		for (int i = 0; i < id; i++)
		{
			if (g_client[i].fd < sockfd || !FD_ISSET(g_client[i].fd, &fd_rd))
				continue ;
			int ret_recv = 1;
			bzero(buf, sizeof(buf));
			while (ret_recv == 1)
			{
				ret_recv = recv(g_client[i].fd, buf + strlen(buf), 1, 0);
				if (buf[strlen(buf) - 1] == '\n')
					break ;
			}
			if (ret_recv == 0)
			{
				sprintf(msg, "server: client %d just left\n", g_client[i].id);
				send_all(g_client[i].id);
				FD_CLR(g_client[i].fd, &curr_fd);
				close(g_client[i].fd);
				g_client[i].id = -1;
				g_client[i].fd = -1;
			}
			else if (ret_recv)
				extract_msg(g_client[i].id);
		}
	}
	return (0);
}