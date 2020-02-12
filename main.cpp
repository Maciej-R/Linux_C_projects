#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <regex.h>
#include <poll.h>
#include <errno.h>
#include <sys/stat.h>
#define _DEBUG

/*
Maciej Ryœnik
24.11.2019

Given no arguments program listens on all interfaces
Given interface name porgram listens on first address found on interface
Given IP address porgram listens on that IP
By default port is 80 (not configurable, hard coded value)
*/

union ip_address {

	//unsigned char   s6_addr[16];
	struct sockaddr_in6* ipv6;
	//uint32_t
	struct sockaddr_in* ipv4;

};

enum choice_type {

	interface,
	address

};

int main(int argc, char** argv) {

	//Create socket
	int socket_descriptor = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	choice_type type;
	//Program parameters if spcified
	ip_address addr;

	//Listening port
	struct sockaddr* target = (sockaddr*)malloc(sizeof(sockaddr));
	//Port for answers to accept() function
	struct sockaddr* peer_socket = (sockaddr*)malloc(sizeof(sockaddr));

	//No input arguments, listen on all 
	if (argc == 1) {

		//Bind all available IPv4 and IPv6 addresses to socket
		struct sockaddr_in *dst = (struct sockaddr_in *) target;
		memset(dst, 0, INET_ADDRSTRLEN);
		dst->sin_family = AF_INET;
		dst->sin_port = htons(80);
		dst->sin_addr.s_addr = INADDR_ANY;

		memcpy(peer_socket, &target, sizeof(sockaddr));
		dst = (struct sockaddr_in*) peer_socket;
		dst->sin_port = htons(2555);

	}
	else {

		regex_t reg;
		regmatch_t match[4];
		size_t n_match = 4;
		const char* expr = "^(eth([0-9]){1,4}$)";
		char* inter;
			   
		//Interface name regex match
		if (!regcomp(&reg, expr, REG_EXTENDED)) {

			int m = regexec(&reg, argv[1], n_match, match, 0);
			n_match = reg.re_nsub;

#ifdef _DEBUG
			//rm_so - The offset in string of the beginning of a substring. Add this value to string to get the address of that part.
			//rm_eo - The offset in string of the end of the substring.
			if (m == 0) {

				printf("%.*s\n", match[1].rm_eo - match[1].rm_so, &argv[1][match[1].rm_so]);

			}
#endif
			if (m == 0) {

				//Saving string of choosen interface name
				int len = match[1].rm_eo - match[1].rm_so + 1;
				inter = (char*)malloc(len);
				memcpy(inter, &argv[1][match[1].rm_so], len);
				inter[len - 1] = '\0';
				type = interface;

			}

		}
		else {

			perror("regex_comp");
			exit(EXIT_FAILURE);

		}

		expr = "^(([0-9]){1,3}.){3}([0-9]){1,3}$";

		//IPv4 address regex match
		if (!regcomp(&reg, expr, REG_EXTENDED)) {

			int m = regexec(&reg, argv[1], n_match, match, 0);
			n_match = reg.re_nsub;

#ifdef _DEBUG

			if (m == 0) {

				printf("%.*s\n", match[0].rm_eo - match[0].rm_so, &argv[1][match[0].rm_so]);

			}
#endif
			if (m == 0) {

				int len = match[0].rm_eo - match[0].rm_so + 1;
				inter = (char*)malloc(len);
				memcpy(inter, &argv[1][match[0].rm_so], len);
				inter[len - 1] = '\0';
				type = address;

			}

		}
		else {

			perror("regex_comp");
			exit(EXIT_FAILURE);

		}

		expr = "^((:)?([0-9a-fA-F]){0,4}:){0,7}([0-9a-fA-F]){0,4}$";

		//IPv6 address regex match
		if (!regcomp(&reg, expr, REG_EXTENDED)) {

			int m = regexec(&reg, argv[1], n_match, match, 0);
			n_match = reg.re_nsub;

#ifdef _DEBUG

			if (m == 0) {

				printf("%.*s\n", match[0].rm_eo - match[0].rm_so, &argv[1][match[0].rm_so]);

			}
#endif
			if (m == 0) {

				printf("IPv6 may cause: \"Address family not supported by protocol\"\n error from bind function\nrequires additonal config");
				int len = match[0].rm_eo - match[0].rm_so + 1;
				inter = (char*)malloc(len);
				memcpy(inter, &argv[1][match[0].rm_so], len);
				inter[len - 1] = '\0';
				type = address;

			}

		}
		else {

			perror("regex_comp");
			exit(EXIT_FAILURE);

		}

		regfree(&reg);

		//List head
		struct ifaddrs* ifaddr;
		//Getting linked list of local interfaces 
		if (getifaddrs(&ifaddr) == -1) {

			perror("getifaddrs");
			exit(EXIT_FAILURE);

		}
			   
		{//Look for target matching input request

			//Local iterator
			struct ifaddrs *ifa;
			bool found = false;

			for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {

				//Only addressed interfaces
				if (ifa->ifa_addr == NULL)
					continue;
				//Only interfaces specified by program input
				if (type == interface && *(ifa->ifa_name) != *inter)
					continue;
				//Address requires additional processing - checks further

				int family = ifa->ifa_addr->sa_family;

				//Assuming usage only of IP connections
				if (family == AF_INET) {

					//Save IP address
					addr.ipv4 = (sockaddr_in *)(ifa->ifa_addr);
					//Set fields in descriptor of target
					struct sockaddr_in *dst = (struct sockaddr_in *) target;
					memset(dst, 0, INET_ADDRSTRLEN);
					dst->sin_family = AF_INET;
					dst->sin_port = htons(80);
					dst->sin_addr = addr.ipv4->sin_addr;
					char* buf = (char*)malloc(INET_ADDRSTRLEN);
					inet_ntop(AF_INET, &addr.ipv4->sin_addr, buf, INET_ADDRSTRLEN);

#ifdef _DEBUG
					char* str = (char*)malloc(INET6_ADDRSTRLEN);
					inet_ntop(AF_INET, &(addr.ipv4->sin_addr), str, INET_ADDRSTRLEN);
					printf("%s\n", str);
					//Bugs with no output occur unless fflush is used
					fflush(stdout);
					free(str);

#endif // _DEBUG
					if (type == address && !strcmp(inter, buf)) {
						found = true;
						break;
					}
					//Listen on first address on given interface
					if (type == interface) {
						found = true;
						break;
					}
					free(buf);

					memcpy(peer_socket, &target, sizeof(sockaddr));
					dst = (struct sockaddr_in*) peer_socket;
					dst->sin_port = htons(2555);

				}
				else if (family == AF_INET6) {

					addr.ipv6 = (sockaddr_in6 *)(ifa->ifa_addr);
					struct sockaddr_in6 *dst = (struct sockaddr_in6 *) target;
					memset(dst, 0, INET6_ADDRSTRLEN);
					dst->sin6_family = AF_INET6;
					dst->sin6_port = htons(80);
					dst->sin6_addr = addr.ipv6->sin6_addr;
					char* buf = (char*)malloc(INET6_ADDRSTRLEN);
					inet_ntop(AF_INET6, &addr.ipv6->sin6_addr, buf, INET6_ADDRSTRLEN);

#ifdef _DEBUG
					char* str = (char*)malloc(INET6_ADDRSTRLEN);
					inet_ntop(AF_INET6, &(addr.ipv6->sin6_addr), str, INET6_ADDRSTRLEN);
					printf("%s\n", str);
					fflush(stdout);
					free(str);

#endif // _DEBUG

					if (type == address && !strcmp(inter, buf)) {
						found = true;
						break;
					}
					if (type == interface) {
						found = true;
						break;
					}
					free(buf);

					memcpy(peer_socket, &target, sizeof(sockaddr));
					dst = (struct sockaddr_in6*) peer_socket;
					dst->sin6_port = htons(2555);

				}


			}

			if (!found) {

				perror("Wrong input");
				exit(EXIT_FAILURE);

			}

			free(inter);

		}//End lookup

		//Free allocated memory
		freeifaddrs(ifaddr);

	}

	//Assign address to socket
	if (bind(socket_descriptor, target, sizeof(sockaddr)) == -1) {

		printf(strerror(errno));
		printf("\n");
		perror("bind\n");
		exit(EXIT_FAILURE);

	}

	//Listen on port
	if (listen(socket_descriptor, 10) == -1) {

		printf(strerror(errno));
		printf("\n");
		perror("listen");
		exit(EXIT_FAILURE);

	}
	else {

		//Print out listening info
		if (addr.ipv4->sin_family == AF_INET) {

			char* str = (char*)malloc(INET_ADDRSTRLEN);
			inet_ntop(AF_INET, &(addr.ipv4->sin_addr), str, INET_ADDRSTRLEN);
			printf("Listenig on address %s, port 80\n", str);
			free(str);

		}
		else {

			char* str = (char*)malloc(INET6_ADDRSTRLEN);
			inet_ntop(AF_INET6, &(addr.ipv6->sin6_addr), str, INET6_ADDRSTRLEN);
			printf("Listenig on address %s, port 80\n", str);
			free(str);

		}

	}
	
	//HTTP header response
	socklen_t peer_addr_size = sizeof(struct sockaddr_in);
	const char* response = "HTTP/1.0 200 OK\nContent-Type:text/html\n\n";
	int res_len = strlen(response);

	//Site file reading
	FILE* site = fopen("/root/projects/SO/site.html", "r");
	if (site == NULL) exit(0);
	struct stat st;
	fstat(fileno(site), &st);
	int flen = st.st_size;
	char* site_buf = (char*)malloc(flen);
	fread(site_buf, 1, flen, site);
	//File not needed any more, loaded to buffor
	fclose(site);

	while (1) {

		//Input of q closes program
		struct pollfd fds;
		int nfds = 1;
		fds.fd = STDIN_FILENO;
		fds.events = POLLIN; //Data to read present
		printf("%s", site_buf);
		if (poll(&fds, nfds, 0)) {

			char* buf = (char*)malloc(5);
			memset(buf, 0, 5);
			buf[4] = '\0';
			scanf("$4s", buf);
			const char* pattern = "q";
			if (strcpy(buf, pattern))
				break;

		}
		fflush(stdout);
		//Waiting for incoming connection request
		//Should be started in separate thread because it's blocking call 
		//and console input is ignored while there's no incoming connections
		int accepted_descriptor = accept(socket_descriptor, peer_socket, &peer_addr_size);

		//Sending out header and site code
		write(accepted_descriptor, response, res_len);
		write(accepted_descriptor, site_buf, flen);

		//Closing connection
		shutdown(accepted_descriptor, SHUT_RDWR);
		close(accepted_descriptor);

	}

	//printf("hello from SO!\n");
	return 0;
}


/*if (family == AF_INET || family == AF_INET6) {

	s = getnameinfo(ifa->ifa_addr,
		(family == AF_INET) ? sizeof(struct sockaddr_in) :
		sizeof(struct sockaddr_in6),
		host, NI_MAXHOST,
		NULL, 0, NI_NUMERICHOST);

	if (s != 0) {

		printf("getnameinfo() failed: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);

	}

	printf("\t\taddress: <%s>\n", host);

}*/