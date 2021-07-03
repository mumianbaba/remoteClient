 

 all:
	gcc term_echo.c term_history.c client.c  client_config.c telnet_cmd.c -lpthread -o term
	rm ecos-remote-client*.tar.gz -rf
	cp term ./release
	./script/pack_ecos_remote_client.sh

clean:
	rm *.o term -rf
	rm ecos-remote-client*.tar.gz -rf
