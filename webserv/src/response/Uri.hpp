#ifndef URI_HPP
#define URI_HPP

#include <string>
#include <ostream>

struct Uri
{
private:
	std::string originUri;

public:
	/**
	 * http://example.com/over/here/index.php/posts/42?name=seushin#home
	 * \____/\__________/\         \________/\_______/\___________/\___/
	 * scheme  authority  \           file    pathInfo    query   fragment
	 *                     \_______________________/
	 *                                path
	 */
	std::string path;
	std::string file;
	std::string extension;
	std::string pathInfo;
	std::string query;
	std::string fragment;
	std::pair<std::string, std::string> root;

public:
	Uri();
	Uri(const std::string &uri);
	Uri(const Uri &other);
	Uri &operator=(const Uri &rhs);
	~Uri();

	std::string getUri() const;
	std::string getServerPath() const;

	void setRoot(const std::string &location, const std::string &root);
	void setIndexFile(const std::string &indexFile);

private:
	std::string trimAuthority(std::string uri) const;
	void applyNewPath(std::string newPath);
	bool isPrefixWith(const std::string &str, const std::string &prefix) const;
	bool isSuffixWith(const std::string &str, const std::string &suffix) const;
};

std::ostream &operator<<(std::ostream &o, const Uri &uri);

#endif // !URI_HPP
