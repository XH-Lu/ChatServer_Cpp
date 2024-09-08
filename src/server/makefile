OUTPUT_DIR = ../../bin
$(OUTPUT_DIR)/ChatServer:main.cpp net/chatserver.cpp service/chatservice.cpp db/db.cpp model/usermodel.cpp model/offlinemsgmodel.cpp model/friendmodel.cpp model/groupmodel.cpp
	g++ $+ -o $@ -g -I ../../include -I ../../include/db -I ../../include/model -I ../../include/net -I ../../include/service -lmuduo_net -lmuduo_base -lmysqlclient -lpthread -std=c++11