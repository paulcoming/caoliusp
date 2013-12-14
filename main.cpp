#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/xpressive/xpressive.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>

namespace ba = boost::asio;
namespace bs = boost::system;
namespace bx = boost::xpressive;
namespace bg = boost::gregorian;

ba::io_service ios;
static const char* cl_host = "cl.or.gs";
static const char* home_url = "thread0806.php?fid=16&search=1";

bool verbose = false;
bg::date ge_day;
int name_c;

bx::sregex node_reg = bx::sregex::compile("<tr align=\"center\" class=\"tr3 t_one\" "
	"onMouseOver=\"this.className='tr3 t_two'\" onMouseOut=\"this.className='tr3 t_one'\">.*?</tr>");
bx::sregex date_reg = bx::sregex::compile("<div class=\"f10\">(.*?)</div>");
bx::sregex uri_reg = bx::sregex::compile("href=\"(.*?)\"");
bx::sregex pic_reg = bx::sregex::compile("input type='image' src='(.*?)'");
bx::sregex remote_pic_reg = bx::sregex::compile("http://(.*?)/(.*(\\..*))");

void request_page(const std::string& host, const std::string& path, std::string& page);

void get_pic(const std::string& uri) {
	std::string page;
	request_page(cl_host, uri, page);

	int t[] = {1};
	bx::sregex_token_iterator it(page.begin(), page.end(), pic_reg, t);
	bx::sregex_token_iterator end;

	for (; it != end; ++it) {
		bx::smatch what;
		if (!bx::regex_match(*it, what, remote_pic_reg))
			continue;

		if (verbose)
			std::cout << "get picture: " << *it << std::endl;

		std::string pic;
		pic.reserve(64 * 1024);
		request_page(what[1], what[2], pic);

		boost::iterator_range<std::string::iterator> rng = boost::find_first(pic, "\r\n\r\n");
		if (rng) {
			std::string name("./pic/");
			name += boost::lexical_cast<std::string>(++name_c);
			name += what[3];
			std::fstream file(name.c_str(), std::ios::out);
			file.write(&pic[0] + std::distance(pic.begin(), rng.end()), pic.size());
		}
	}
}

void parse(const std::string& node, std::vector<std::string>& uri) {
	bx::smatch what;
	if (bx::regex_search(node, what, date_reg)) {
		if (verbose) {
			for (size_t i = 0; i < what.size(); i++)
				std::cout << "regex-match#" << i << " " << what[i] << std::endl;
		}

		if (what[1] != "Top-marks" && bg::from_string(what[1]) >= ge_day) {
			bx::smatch uri_match;
			if (bx::regex_search(node, uri_match, uri_reg))
				uri.push_back(uri_match[1]);
		}
	} else {
		if (verbose)
			std::cout << "didn't find date" << std::endl;
	}
}

void parse_latest_page(const std::string& page) {
	bx::sregex_token_iterator it(page.begin(), page.end(), node_reg);
	bx::sregex_token_iterator end;
	std::vector<std::string> uri;

	for (; it != end; ++it) {
		if (verbose) {
			std::cout << "---------------------" << std::endl;
			std::cout << *it << std::endl;
		}

		parse(*it, uri);
	}

	std::cout << "--------today's latest post (total " << uri.size() << " posts)--------" << std::endl;
	for (std::vector<std::string>::iterator it = uri.begin(); it != uri.end(); ++it)
		std::cout << *it << std::endl;

	int i = 0;
	std::cout << "\n--------------------\n";
	for (std::vector<std::string>::iterator it = uri.begin(); it != uri.end(); ++it) {
		std::cout << "get pictures from " << *it << " #" << ++i << std::endl;
		get_pic(*it);
	}
}

void request_page(const std::string& host, const std::string& path, std::string& page) {
	std::string uri("GET /");
	uri += path;
	uri += " HTTP/1.1\r\n";
	uri += "Host: ";
	uri += host;
	uri += "\r\n";
	uri += "User-Agent: Chome\r\n\r\n";

	ba::ip::tcp::resolver res(ios);
	ba::ip::tcp::resolver::query qry(host, "80");
	ba::ip::tcp::resolver::iterator it = res.resolve(qry);

	ba::ip::tcp::socket sock(ios);
	ba::connect(sock, it);
	ba::write(sock, ba::buffer(uri));

	char buf[1024];
	int len;
	bs::error_code err;
	while (!err) {
		len = sock.read_some(ba::buffer(buf), err);
		page.append(buf, len);
	}
}

int main(int argc, char* argv[]) {
	if (argc > 1)
		verbose = true;

	// for alpha, must add a parameter about day
	ge_day = bg::from_string("2013-12-14");

	std::string page;
	request_page(cl_host, home_url, page);
	if (verbose)
		std::cout << "page:\n" << page << std::endl;

	parse_latest_page(page);

	return 0;
}