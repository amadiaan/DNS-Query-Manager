# DNS-Query-Manager
complie command

gcc -o dns  $(mysql_config --cflags) dns.cpp $(mysql_config --libs)
gcc -o dns  $(mysql_config --cflags) dns.cpp $(mysql_config --libs) -D VEROBOSE
