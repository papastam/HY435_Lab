HEADERS = inc/mini_iperf.h inc/util_crc32.h inc/utils.h 
OBJECTS = obj/utils.o obj/mini_iperf.o obj/mini_iperf_client.o obj/mini_iperf_server.o

FLAGS = -I inc/ -I src/ -g

default: mini_iperf

obj/%.o: src/%.c $(HEADERS)
	gcc $(FLAGS) -c $< -o $@ -lm

mini_iperf: $(OBJECTS)
	gcc $(FLAGS) $(OBJECTS) -o $@ -lm -pthread

clean:
	-rm -f obj/*.o
	-rm -f mini_iperf

cd: mini_iperf
	./mini_iperf -c -a 127.0.0.1 -p 55000 -i 1 -b 100M -d -n 1 -t 20

sd: mini_iperf
	./mini_iperf -s -a 127.0.0.1 -p 55000 -i 1 

c: mini_iperf
	./mini_iperf -c -a 127.0.0.1 -p 55000 -i 1 -l 12000 -b 900M -n 1 -t 20 

s: mini_iperf
	./mini_iperf -s -a 127.0.0.1 -p 55000 -i 1 

cp: mini_iperf
	./mini_iperf -c -a 127.0.0.1 -p 55000 -l 1000 -b 100000 -n 1 -t 20 -i 1

sp: mini_iperf
	./mini_iperf -s -a 127.0.0.1 -p 55000

ct: mini_iperf
	./mini_iperf -c -a 127.0.0.1 -p 54000 -i 4 -l 7000 -b 550M -n 17 -t 20

st: mini_iperf
	./mini_iperf -s -a 127.0.0.1 -p 54000 -i 1

iftop: 
	sudo iftop -i lo
