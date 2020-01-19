/**
 * SPIFFS webserver
 */
bool exists(String path) {
	bool yes = false;
	File file = SPIFFS.open(path, "r");
	if (!file.isDirectory()) {
		yes = true;
	}
	file.close();
	return yes;
}

void handleMinimalUpload() {
	server.sendHeader("Access-Control-Allow-Origin", "*");
	server.send(200, "text/html", F("<!DOCTYPE html>\
			<html>\
			<head>\
				<title>ESP8266 Upload</title>\
				<meta charset=\"utf-8\">\
				<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\">\
				<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
			</head>\
			<body>\
				<form action=\"/fs/upload\" method=\"post\" enctype=\"multipart/form-data\">\
				<input type=\"file\" name=\"data\">\
				<input type=\"text\" name=\"path\" value=\"/\">\
				<button>Upload</button>\
				</form>\
			</body>\
			</html>"));
}

void handleFileUpload() {
	File fsUploadFile;
	HTTPUpload &upload = server.upload();
	if (upload.status == UPLOAD_FILE_START) 	{
		String filename = upload.filename;
		if (!filename.startsWith("/"))
		{
			filename = "/" + filename;
		}
		DBG_PRINT("handleFileUpload Name: ");
		DBG_PRINTLN(filename);
		fsUploadFile = SPIFFS.open(filename, "w");
		filename = String();
	} else if (upload.status == UPLOAD_FILE_WRITE) {
		//DBG_OUTPUT_PORT.print("handleFileUpload Data: "); DBG_OUTPUT_PORT.println(upload.currentSize);
		if (fsUploadFile) {
			fsUploadFile.write(upload.buf, upload.currentSize);
		}
	} else if (upload.status == UPLOAD_FILE_END) {
		if (fsUploadFile) {
			fsUploadFile.close();
		}
		DBG_PRINT("handleFileUpload Size: ");
		DBG_PRINTLN(upload.totalSize);
	}
}

void handleFileDelete() {
	if (server.args() == 0)	{
		return server.send(500, "text/plain", "BAD ARGS");
	}
	String path = server.arg(0);
	DBG_PRINTLN("handleFileDelete: " + path);
	if (path == "/") {
		return server.send(500, "text/plain", "BAD PATH");
	}
	if (!exists(path)) {
		return server.send(404, "text/plain", "FileNotFound");
	}
	SPIFFS.remove(path);
	server.send(200, "text/plain", "");
	path = String();
}

void handleFileList() {
	if (!server.hasArg("dir")) {
		server.send(500, "text/plain", "BAD ARGS");
		return;
	}

	String path = server.arg("dir");
	DBG_PRINTLN("handleFileList: " + path);

	File root = SPIFFS.open(path);
	path = String();
	String output = "[";
	if (root.isDirectory()) {
		File file = root.openNextFile();
		while (file) {
			if (output != "[") {
				output += ',';
			}
			output += "{\"type\":\"";
			output += (file.isDirectory()) ? "dir" : "file";
			output += "\",\"name\":\"";
			output += String(file.name()).substring(1);
			output += "\"}";
			file = root.openNextFile();
		}
	}
	output += "]";
	server.send(200, "text/json", output);
}

String getContentType(String filename) {
	if (server.hasArg("download")) {
		return "application/octet-stream";
	} else if (filename.endsWith(".htm"))	{
		return "text/html";
	} else if (filename.endsWith(".html")) {
		return "text/html";
	} else if (filename.endsWith(".css")) {
		return "text/css";
	} else if (filename.endsWith(".js")) {
		return "application/javascript";
	} else if (filename.endsWith(".png")) {
		return "image/png";
	} else if (filename.endsWith(".gif")) {
		return "image/gif";
	} else if (filename.endsWith(".jpg"))	{
		return "image/jpeg";
	} else if (filename.endsWith(".ico")) {
		return "image/x-icon";
	} else if (filename.endsWith(".xml")) {
		return "text/xml";
	} else if (filename.endsWith(".pdf"))	{
		return "application/x-pdf";
	} else if (filename.endsWith(".zip")) {
		return "application/x-zip";
	} else if (filename.endsWith(".gz")) {
		return "application/x-gzip";
	}
	return "text/plain";
}

bool handleFileRead(String path) {
	DBG_PRINTLN("handleFileRead: " + path);
	if (path.endsWith("/"))	{
		path += "index.htm";
	}
	String contentType = getContentType(path);
	String pathWithGz = path + ".gz";
	if (exists(pathWithGz) || exists(path))	{
		if (exists(pathWithGz))	{
			path += ".gz";
		}
		File file = SPIFFS.open(path, "r");
		server.streamFile(file, contentType);
		file.close();
		return true;
	}
	return false;
}