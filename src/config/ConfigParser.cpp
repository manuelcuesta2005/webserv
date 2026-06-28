#include "webserv.hpp"
#include <fstream>
#include <sstream>
#include <cstdlib>

ConfigParser::ConfigParser()
    : _tokens(), _pos(0)
{}


// -------------------------------------------------------------
//  readFile: lee el archivo entero a un std::string. Hecho.
//  Si el archivo no existe o no se puede abrir, lanza ConfigError.
// -------------------------------------------------------------
std::string ConfigParser::readFile(const std::string& path)
{
    std::ifstream in(path.c_str());
    if (!in)
        throw ConfigError("cannot open file: " + path);
    std::ostringstream oss;
    oss << in.rdbuf();
    return oss.str();
}


// -------------------------------------------------------------
//  peek(): devuelve el token actual sin consumirlo. Hecho.
// -------------------------------------------------------------
const Token& ConfigParser::peek() const
{
    return _tokens[_pos];
}

const Token& ConfigParser::consume()
{
    // TODO
    const Token& tok = peek();
    ++_pos;
    return tok;
}


// -------------------------------------------------------------
//  expect(type, errMsg): comprueba el tipo del token actual.
//    throw ConfigError(errMsg, peek().line, peek().column);
//
//  Esto crea un ConfigError nuevo y lo lanza como excepción.
//  El que llamó a expect() puede capturarla con try/catch o
//  dejarla propagar hacia arriba.
// -------------------------------------------------------------
const Token& ConfigParser::expect(TokenType type, const std::string& errMsg)
{
    // TODO
    if (peek().type != type)
        throw ConfigError(errMsg, peek().line, peek().column);
    return consume();
}


// -------------------------------------------------------------
//  match(type): comprueba el tipo sin consumir.
//
//  Devuelve si peek().type es igual al tipo dado.
// -------------------------------------------------------------
bool ConfigParser::match(TokenType type) const
{
    // TODO
    return (peek().type == type);
}


// -------------------------------------------------------------
//  matchWord(word): comprueba si el token actual es un WORD
//  con el valor textual dado.
//
//  Para comparar dos std::string: a == b funciona. No uses strcmp.
// -------------------------------------------------------------
bool ConfigParser::matchWord(const std::string& word) const
{
    // TODO
    if (peek().type == TOK_WORD && peek().value == word)
        return (true);
    return (false);
}


// -------------------------------------------------------------
//  Lee el archivo, lo tokeniza, guarda los tokens en _tokens,
//  y lanza el parsing recursivo desde parseGlobal().
// -------------------------------------------------------------
GlobalConfig ConfigParser::parse(const std::string& path)
{
    std::string source = readFile(path);

    ConfigTokenizer tk(source);
    _tokens = tk.tokenize();
    _pos = 0;

    return parseGlobal();
}


// -------------------------------------------------------------
//  parseGlobal: pendiente. Devuelve algo vacío para que compile.
// -------------------------------------------------------------
GlobalConfig ConfigParser::parseGlobal()
{
    GlobalConfig cfg;

    while (!match(TOK_EOF))
    {
        if (matchWord("server"))
            cfg.servers.push_back(parseServerBlock());
        else
            throw ConfigError("expected 'server' block",
                peek().line, peek().column);
    }
    return cfg;
}


// -------------------------------------------------------------
//  parseServerBlock: pendiente.
// -------------------------------------------------------------
ServerConfig ConfigParser::parseServerBlock()
{
    // TODO en un paso siguiente
    expect(TOK_WORD, "expected 'server'");
    expect(TOK_LBRACE, "expected '{' after 'server'");

    ServerConfig srv;
    bool seenClientMaxBody = false;

    while (!match(TOK_RBRACE) && !match(TOK_EOF))
        parseServerDirective(srv, seenClientMaxBody);
    
    expect(TOK_RBRACE, "expected '}' to close 'server' block");
    
    if (srv.listen.empty())
        throw ConfigError("server block has no 'listen' directive",
            peek().line, peek().column);

    return srv;
}


// -------------------------------------------------------------
//  parseServerDirective: pendiente.
// -------------------------------------------------------------
void ConfigParser::parseServerDirective(ServerConfig& srv, bool& seenClientMaxBody)
{
    if (matchWord("listen"))
    {
        consume();
        const Token& val = expect(TOK_WORD, "expected host:port or port after 'listen'");
        srv.listen.push_back(parseListenValue(val));
        expect(TOK_SEMICOLON, "expected ';' after 'listen' value");
    }
    else if (matchWord("server_name"))
    {
        consume();
        while (match(TOK_WORD))
            srv.server_names.push_back(consume().value);
        expect(TOK_SEMICOLON, "expected ';' after 'server_name' values");
    }
    else if (matchWord("client_max_body_size"))
    {
        if (seenClientMaxBody)
            throw ConfigError("'client_max_body_size' duplicated in server block",
                              peek().line, peek().column);
        seenClientMaxBody = true;
        consume();
        const Token& val = expect(TOK_WORD, "expected size after 'client_max_body_size'");
        srv.client_max_body_size = parseSize(val);
        expect(TOK_SEMICOLON, "expected ';' after 'client_max_body_size'");
    }
    else if (matchWord("error_page"))
    {
        consume();
        const Token& codeTok = expect(TOK_WORD, "expected error code after 'error_page'");
        const Token& pathTok = expect(TOK_WORD, "expected path after error code");
        int code = parseInt(codeTok, 100, 599, "invalid error page code");
        srv.error_pages[code] = pathTok.value;
        expect(TOK_SEMICOLON, "expected ';' after 'error_page'");
    }
    else if (matchWord("location"))
    {
        srv.locations.push_back(parseLocationBlock());
    }
    else if (matchWord("root"))
    {
        throw ConfigError("'root' is not allowed at server level (only inside location)",
                          peek().line, peek().column);
    }
    else
    {
        throw ConfigError("unknown directive '" + peek().value + "' in server block",
                          peek().line, peek().column);
    }
}


// -------------------------------------------------------------
//  parseLocationBlock: pendiente.
// -------------------------------------------------------------
LocationConfig ConfigParser::parseLocationBlock()
{
    expect(TOK_WORD, "expected 'location'");
    const Token& pathTok = expect(TOK_WORD, "expected path after 'location'");
    expect(TOK_LBRACE, "expected '{' after location path");

    LocationConfig loc;
    loc.path = pathTok.value;

    bool seenClientMaxBody = false;

    while (!match(TOK_RBRACE) && !match(TOK_EOF))
        parseLocationDirective(loc, seenClientMaxBody);

    expect(TOK_RBRACE, "expected '}' to close 'location' block");
    return loc;
}


// -------------------------------------------------------------
//  parseLocationDirective: pendiente.
// -------------------------------------------------------------
void ConfigParser::parseLocationDirective(LocationConfig& loc, bool& seenClientMaxBody)
{
    if (matchWord("root"))
    {
        consume();
        loc.root = expect(TOK_WORD, "expected path after 'root'").value;
        expect(TOK_SEMICOLON, "expected ';' after 'root'");
    }
    else if (matchWord("methods"))
    {
        consume();
        while (match(TOK_WORD))
        {
            const Token& m = consume();
            if (m.value != "GET" && m.value != "POST" && m.value != "DELETE")
                throw ConfigError("unknown HTTP method '" + m.value + "'",
                                  m.line, m.column);
            loc.allowed_methods.insert(m.value);
        }
        expect(TOK_SEMICOLON, "expected ';' after 'methods' values");
    }
    else if (matchWord("index"))
    {
        consume();
        while (match(TOK_WORD))
            loc.index_files.push_back(consume().value);
        expect(TOK_SEMICOLON, "expected ';' after 'index' values");
    }
    else if (matchWord("autoindex"))
    {
        consume();
        const Token& val = expect(TOK_WORD, "expected 'on' or 'off' after 'autoindex'");
        if (val.value == "on")
            loc.autoindex = true;
        else if (val.value == "off")
            loc.autoindex = false;
        else
            throw ConfigError("expected 'on' or 'off' for 'autoindex'",
                              val.line, val.column);
        expect(TOK_SEMICOLON, "expected ';' after 'autoindex'");
    }
    else if (matchWord("return"))
    {
        consume();
        const Token& codeTok = expect(TOK_WORD, "expected redirect code after 'return'");
        const Token& targetTok = expect(TOK_WORD, "expected redirect target after code");
        loc.redirect_code = parseInt(codeTok, 300, 399, "invalid redirect code");
        loc.redirect_target = targetTok.value;
        expect(TOK_SEMICOLON, "expected ';' after 'return'");
    }
    else if (matchWord("upload_store"))
    {
        consume();
        loc.upload_store = expect(TOK_WORD, "expected path after 'upload_store'").value;
        loc.upload_enabled = true;
        expect(TOK_SEMICOLON, "expected ';' after 'upload_store'");
    }
    else if (matchWord("cgi_extension"))
    {
        consume();
        const Token& ext    = expect(TOK_WORD, "expected extension after 'cgi_extension'");
        const Token& interp = expect(TOK_WORD, "expected interpreter after extension");
        loc.cgi_handlers[ext.value] = interp.value;
        expect(TOK_SEMICOLON, "expected ';' after 'cgi_extension'");
    }
    else if (matchWord("client_max_body_size"))
    {
        if (seenClientMaxBody)
            throw ConfigError("'client_max_body_size' duplicated in location block",
                              peek().line, peek().column);
        seenClientMaxBody = true;
        consume();
        const Token& val = expect(TOK_WORD, "expected size after 'client_max_body_size'");
        loc.client_max_body_size = parseSize(val);
        expect(TOK_SEMICOLON, "expected ';' after 'client_max_body_size'");
    }
    else
    {
        throw ConfigError("unknown directive '" + peek().value + "' in location block",
                          peek().line, peek().column);
    }
}

ListenEndpoint ConfigParser::parseListenValue(const Token& tok)
{
    const std::string& val = tok.value;
    std::string host = "0.0.0.0";
    std::string portStr;

    std::size_t colon = val.rfind(':');
    if (colon == std::string::npos)
    {
        portStr = val;
    }
    else
    {
        host    = val.substr(0, colon);
        portStr = val.substr(colon + 1);
        if (host == "localhost")
            host = "127.0.0.1";
    }

    for (std::size_t i = 0; i < portStr.size(); ++i)
    {
        if (portStr[i] < '0' || portStr[i] > '9')
            throw ConfigError("invalid port '" + portStr + "'",
                              tok.line, tok.column);
    }
    long port = std::strtol(portStr.c_str(), NULL, 10);
    if (port < 1 || port > 65535)
        throw ConfigError("invalid port number (out of range 1-65535)",
                          tok.line, tok.column);

    return ListenEndpoint(host, static_cast<uint16_t>(port));
}

long ConfigParser::parseSize(const Token& tok)
{
    const std::string& s = tok.value;
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            throw ConfigError("invalid size value '" + s + "'",
                              tok.line, tok.column);
    }
    return std::strtol(s.c_str(), NULL, 10);
}

int ConfigParser::parseInt(const Token& tok, int minVal, int maxVal,
                           const std::string& errMsg)
{
    const std::string& s = tok.value;
    for (std::size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] < '0' || s[i] > '9')
            throw ConfigError(errMsg + " '" + s + "'",
                              tok.line, tok.column);
    }
    long v = std::strtol(s.c_str(), NULL, 10);
    if (v < minVal || v > maxVal)
        throw ConfigError(errMsg, tok.line, tok.column);
    return static_cast<int>(v);
}