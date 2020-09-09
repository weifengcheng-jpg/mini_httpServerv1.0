all: mini_http.exe

mini_http.exe:mini_http.c
	gcc mini_http.c -o mini_http.exe

clean:
	rm -f mini_http.exe