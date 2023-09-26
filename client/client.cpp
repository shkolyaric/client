#include<iostream>
//библиотека asio для работы с сетью
#include<boost/asio.hpp>
//библиотеки для работы с json
#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/json_parser.hpp>
//библиотека с умными указателями
#include<memory>
#include<cctype>//библиотека с методом toupper()
#include<string>
//библиотеки для логирования
#include <boost/log/trivial.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/support/date_time.hpp>
#include <fstream>
#include <boost/log/utility/setup/common_attributes.hpp>
//метод который возвращает поле из JSON
std::string getfield(std::string json,size_t len, std::string field)
{
    std::stringstream jsonEnc(json.substr(0,len));
    boost::property_tree::ptree root;
    boost::property_tree::read_json(jsonEnc,root);
    return root.get<std::string>(field);
}
//генирируем json в виде строки вида:
/*
* {
*   "req":"запрос к серверу"
* }
*/
std::string genjson(std::string req,bool pret)
{
    //создание дерева свойст
    boost::property_tree::ptree root;
    //добавление нужного поля
    root.put("req", req);
    //записываем json в стринг поток
    std::ostringstream oss;
    boost::property_tree::write_json(oss, root,pret);//третье поле отвечает за "хорошесть"(будут ли переносы строки)
    return oss.str();
}

void init_logger()
{
    namespace logging = boost::log;
    namespace sinks = logging::sinks;
    namespace expr = logging::expressions;
    namespace attrs = logging::attributes;
    using text_sink = sinks::synchronous_sink<sinks::text_ostream_backend>;
    auto sink = boost::make_shared<text_sink>();
    //подключаем выходной файловый поток
    boost::shared_ptr<std::ostream> stream(boost::shared_ptr< std::ostream >(new std::ofstream("run.log")));
    sink->locked_backend()->add_stream(stream);

    // ставим выходной формат
    sink->set_formatter(
        expr::stream
        << '['
        << expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
        << "]: "
        << expr::smessage
    );
    logging::add_common_attributes();

    logging::core::get()->add_sink(sink);
}

class client_apl
{
private:
    std::string mes_;//сообщение для/от сервера
    std::string pret_mes;//"хорошенький" JSON
    boost::asio::ip::tcp::socket socket_;//для отправки сообщений
    boost::asio::ip::tcp::endpoint ends_;//конечный ip и port для подключения
public:
    client_apl(boost::asio::io_context& io, boost::asio::ip::address end, short port, std::string mes) :socket_(io), mes_(genjson(mes, false)),pret_mes(genjson(mes,true)), ends_(end, port)
    {
        BOOST_LOG_TRIVIAL(info) << "initialize connection\n";
        connecting();
    }
private:
    void connecting()
    {       
        socket_.async_connect( ends_,
            [this](std::error_code ec)
            {
                if (!ec)
                {
                    BOOST_LOG_TRIVIAL(info) << "connection already\n";
                    BOOST_LOG_TRIVIAL(info) << "start write\n";
                    writting();
                }
                else
                {
                    BOOST_LOG_TRIVIAL(info) << "connection failed\n";
                    BOOST_LOG_TRIVIAL(info)<<"ERROR:" << ec.message() << '\n';
                }
            });
            
    }
    void writting()
    {
        boost::asio::async_write(socket_, boost::asio::buffer(mes_),
            [this](std::error_code ec, size_t)
            {
                if(!ec)
                {
                    BOOST_LOG_TRIVIAL(info) << "write end\n";
                    BOOST_LOG_TRIVIAL(info) << "JSON sended to server:\n"<<pret_mes<<'\n';
                    BOOST_LOG_TRIVIAL(info) << "start read\n";
                    mes_.clear();
                    mes_.resize(100);
                    reading();
                }
                else
                {
                    BOOST_LOG_TRIVIAL(info) << "write failed\n";
                    BOOST_LOG_TRIVIAL(info) << "ERROR:" << ec.message() << '\n';
                }
            });
    }
    void reading()
    {
        socket_.async_read_some( boost::asio::buffer(mes_),
            [this](std::error_code ec, size_t len)
            {
                if(!ec)
                {
                    BOOST_LOG_TRIVIAL(info) << "read end\n";
                    BOOST_LOG_TRIVIAL(info) << "server send:" << mes_ << '\n';
                    mes_ = getfield(mes_,len,"resp");
                    BOOST_LOG_TRIVIAL(info) << "server response:" << mes_ << '\n';
                    std::cout << mes_ << '\n';
                }
                
                else
                {
                    BOOST_LOG_TRIVIAL(info) << "read failed\n";
                    BOOST_LOG_TRIVIAL(info) << "ERROR:" << ec.message() << '\n';
                }
            });
    }
};
int main()
{
    init_logger();
    char chose;//символьная переменная 
    //UI для нашего юзера
    std::cout << "read from a file or from the console?(F/c)\n";
    std::cin >> chose;
    std::string expr;//строка которая хранит выражение
    //случай если читаем с файла
    if (toupper(chose) == 'F')
    {
        BOOST_LOG_TRIVIAL(info) << "try to read from file(test.txt)\n";
        try
        {
            std::ifstream in("test.txt");
            in >> expr;
            in.close();
            BOOST_LOG_TRIVIAL(info) << "read from file(test.txt) complete\n";
        }
        catch (std::exception& e)
        {
            //что-то не так с файлом роняем программу 
            BOOST_LOG_TRIVIAL(info) << "fail to read from file(test.txt)\n";
            BOOST_LOG_TRIVIAL(info) << "exception:" << e.what()<<'\n';
            return 0;
        }
    }
    //случай если читаем с консоли
    else
    {
        BOOST_LOG_TRIVIAL(info) << "start read from console\n";
        std::cout << "enter the expression:\n";
        std::cin >> expr;
        BOOST_LOG_TRIVIAL(info) << "read from console complete\n";
    }
    BOOST_LOG_TRIVIAL(info) << "expression read:"<<expr<<'\n';
    try
    {
        boost::asio::io_context context;
        auto end = boost::asio::ip::address::from_string("127.0.0.1");
        client_apl apl(context,end,1337,expr);
        context.run();
    }
    catch (std::exception& e)
    {
        BOOST_LOG_TRIVIAL(info) << "failed to start\n";
        BOOST_LOG_TRIVIAL(info) << "exception:" << e.what() << '\n';
    }
}