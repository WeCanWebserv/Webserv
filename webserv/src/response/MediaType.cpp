#include "MediaType.hpp"

MediaType::mimeType MediaType::mime = initMime();

MediaType::mimeType MediaType::initMime()
{
	mimeType mime;

	mime[""] = "text/plain";
	mime[".txt"] = "text/plain";
	mime[".htm"] = "text/html";
	mime[".html"] = "text/html";
	mime[".css"] = "text/css";
	mime[".csv"] = "text/csv";
	mime[".js"] = "text/javascript";
	mime[".mjs"] = "text/javascript";

	mime[".jpg"] = "image/jpeg";
	mime[".jpeg"] = "image/jpeg";
	mime[".bmp"] = "image/bmp";
	mime[".png"] = "image/png";
	mime[".gif"] = "image/gif";
	mime[".svg"] = "image/svg+xml";
	mime[".tif"] = "image/tiff";
	mime[".tiff"] = "image/tiff";
	mime[".ico"] = "image/vnd.microsoft.icon";
	mime[".webp"] = "image/webm";
	mime[".avif"] = "image/avif";

	mime[".aac"] = "audio/aac";
	mime[".mp3"] = "audio/mpeg";
	mime[".oga"] = "audio/ogg";
	mime[".wav"] = "audio/wav";
	mime[".weba"] = "audio/webm";

	mime[".mpeg"] = "video/mpeg";
	mime[".mp4"] = "video/mp4";
	mime[".ogg"] = "video/ogg";
	mime[".avi"] = "video/x-msvideo";
	mime[".webm"] = "video/webm";

	mime[".otf"] = "font/otf";
	mime[".ttf"] = "font/ttf";
	mime[".woff"] = "font/woff";
	mime[".woff2"] = "font/woff2";

	mime[".bin"] = "application/octet-stream";
	mime[".json"] = "application/json";
	mime[".pdf"] = "application/pdf";
	mime[".php"] = "application/x-httpd-php";
	mime[".doc"] = "application/msword";
	mime[".docx"] = "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
	mime[".xls"] = "application/vnd.ms-excel";
	mime[".xlsx"] = "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet";
	mime[".xml"] = "application/xml";
	mime[".rtf"] = "application/rtf";
	mime[".sh"] = "application/x-sh";
	mime[".gz"] = "application/gzip";
	mime[".tar"] = "application/x-tar";
	mime[".zip"] = "application/zip";
	mime[".7z"] = "application/x-7z-compressed";
	mime[".ogx"] = "application/ogg";
	mime[".swf"] = "application/x-shockwave-flash";
	return (mime);
}

std::string MediaType::get(const std::string &extension)
{
	mimeType::const_iterator found;

	found = mime.find(extension);
	if (found == mime.end())
		return ("application/octet-stream");
	return ((*found).second);
}
