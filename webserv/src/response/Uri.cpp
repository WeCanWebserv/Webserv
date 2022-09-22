#include "Uri.hpp"

Uri::Uri() {}

Uri::Uri(const std::string &uri)
{
	this->originUri = trimAuthority(uri);

	std::string::size_type qry, frg;

	frg = this->originUri.find('#');
	if (frg != std::string::npos)
	{
		this->fragment = this->originUri.substr(frg + 1);
	}

	qry = this->originUri.find('?');
	if (qry != std::string::npos)
	{
		this->query = this->originUri.substr(qry + 1, frg - qry - 1);
	}

	this->path = this->originUri.substr(0, qry);
	applyNewPath(this->path);
}

Uri::Uri(const Uri &other) : originUri(other.originUri)
{
	*this = other;
}

Uri &Uri::operator=(const Uri &rhs)
{
	this->path = rhs.path;
	this->file = rhs.file;
	this->extension = rhs.extension;
	this->pathInfo = rhs.pathInfo;
	this->query = rhs.query;
	this->fragment = rhs.fragment;
	this->root = rhs.root;

	return (*this);
}

Uri::~Uri() {}

std::string Uri::getUri() const
{
	return (this->path + this->query + this->fragment);
}

std::string Uri::getServerPath() const
{
	std::string p = this->path;

	p.replace(0, this->root.first.size(), this->root.second);
	return (p);
}

void Uri::setRoot(const std::string &location, const std::string &root)
{
	this->root = std::make_pair(location, root);
}

void Uri::setIndexFile(const std::string &indexFile)
{
	this->path += indexFile;
	applyNewPath(this->path);
}

std::string Uri::trimAuthority(std::string uri) const
{
	std::string path = uri;
	std::string::size_type pos;

	if (isPrefixWith(path, "http://") || isPrefixWith(path, "https://"))
	{
		/**
		 * https://www.example.com/index.html
		 *                        ^
		 *                        find here
		 */
		pos = path.find('/', path.find(':') + 3);
		if (pos == std::string::npos)
			pos = path.size();
		path = path.substr(pos);
	}

	if (!isPrefixWith(path, "/"))
		path.insert(path.begin(), '/');

	return (path);
}

void Uri::applyNewPath(std::string newPath)
{
	std::string::size_type dot;

	dot = newPath.rfind('.');
	if (dot != std::string::npos)
	{
		std::string::size_type pos = newPath.find('/', dot);

		if (pos != std::string::npos)
		{
			this->pathInfo = newPath.substr(pos);
			newPath = newPath.substr(0, pos);
		}
		this->extension = newPath.substr(dot);

		pos = newPath.rfind('/');
		if (pos == std::string::npos)
			pos = 0;
		this->file = newPath.substr(pos);
	}
}

bool Uri::isPrefixWith(const std::string &str, const std::string &prefix) const
{
	const int strSize = str.size();
	const int preSize = prefix.size();

	if (strSize >= preSize)
	{
		if (str.compare(0, preSize, prefix) == 0)
			return (true);
	}
	return (false);
}

bool Uri::isSuffixWith(const std::string &str, const std::string &suffix) const
{
	const int strSize = str.size();
	const int sufSize = suffix.size();

	if (strSize >= sufSize)
	{
		if (str.compare(strSize - sufSize, sufSize, suffix) == 0)
			return (true);
	}
	return (false);
}

std::ostream &operator<<(std::ostream &o, const Uri &uri)
{
	o << "path: " << uri.path << std::endl
		<< "file: " << uri.file << std::endl
		<< "extension: " << uri.extension << std::endl
		<< "pathInfo: " << uri.pathInfo << std::endl
		<< "query: " << uri.query << std::endl
		<< "fragment: " << uri.fragment;
	return (o);
}
