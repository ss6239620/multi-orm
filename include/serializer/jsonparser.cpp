#include "jsonparser.h"

/**
 * @brief Finds matching pairs of opening and closing braces in a given string.
 *
 * @param s The input string containing braces.
 * @param bracePairs A reference to an unordered_map that stores
 *                   matched brace pairs (opening index -> closing index).
 */
void findBarcePairs(const std::string &s, std::unordered_map<int, int> &bracePairs)
{
    std::vector<int> stack;
    int n = s.length();
    for (int i = 0; i < n; i++)
    {
        if (s[i] == '[' || s[i] == '{')
        {
            stack.push_back(i);
        }
        else if (s[i] == ']' || s[i] == '}')
        {
            bracePairs[stack.back()] = i;
            stack.pop_back();
        }
    }
}

/**
 * @brief Checks if a given character is a whitespace character.
 *
 * @param c The character to check.
 * @return true if c is a whitespace character, false otherwise.
 */
bool isWhiteSpace(char c)
{
    return c == ' ' || c == '\n';
}

/**
 * @brief Checks if a given string represents a valid floating-point number (double).
 *
 * @param s The input string to check.
 * @return true if the string represents a valid double, false otherwise.
 */
bool isDouble(const std::string &s)
{
    if (s.empty()) return false;
    int i = 0;
    if (s[0] == '+' || s[0] == '-')
        i++;

    bool dotseen = 0;

    while (i < s.length())
    {
        // if charctot is not number and not . then return false
        if (!std::isdigit(s[i]) && s[i] != '.')
            return false;

        if (s[i] == '.')
        {
            // if dot is already seen return false
            if (dotseen)
                return false;
            dotseen = true;
        }
        i++;
    }
    return true;
}

bool isInteger(const std::string &s)
{
    if (s.empty()) return false;
    int i = 0;
    if (s[0] == '+' || s[0] == '-')
        i++;

    while (i < s.length())
    {
        if (!std::isdigit(s[i]))
            return false;
        i++;
    }
    return true;
}

JSONNode getValue(const std::string &s)
{
    int i = 0, j = s.length() - 1;

    // Trim whitespace
    while (i <= j && isWhiteSpace(s[i])) i++;
    while (j >= i && isWhiteSpace(s[j])) j--;

    if (i > j) return JSONNode(); // Empty string becomes null

    std::string temp = s.substr(i, j - i + 1);

    if (temp.empty()) return JSONNode();

    if (temp[0] == '"') {
        if (temp.length() < 2 || temp.back() != '"') {
            return JSONNode(); // Invalid string
        }
        return JSONNode(temp.substr(1, temp.length() - 2));
    }

    if (temp == "true") return JSONNode(true);
    if (temp == "false") return JSONNode(false);
    if (temp == "null") return JSONNode();

    // Numeric handling
    if (isDouble(temp)) {
        try {
            if (isInteger(temp)) {
                try {
                    return JSONNode(std::stoi(temp));
                } catch (...) {
                    return JSONNode(std::stod(temp));
                }
            }
            return JSONNode(std::stod(temp));
        } catch (...) {
            return JSONNode(temp);
        }
    }

    return JSONNode(temp);
}

JSONNode JSONNode::parse(const std::string &s)
{
    std::unordered_map<int, int> bracePairs;
    findBarcePairs(s, bracePairs); // find all braces { and [ from string s

    int i = 0;

    while (isWhiteSpace(s[i])) // remove whitespaces
        i++;

    // start will be current i and end can be find from bracePairs[i]
    if (s[i] == '[')
        return parseArray(s, i, bracePairs[i], bracePairs);

    return parseObject(s, i, bracePairs[i], bracePairs);
}

JSONNode parseObject(const std::string &s, int start, int end, std::unordered_map<int, int> &bracePair)
{
    int i = start;
    JSONNode ans(JSONType::OBJECT);

    while (i < end)
    {
        // Skip non-quote characters (with bounds check)
        while (i < end && s[i] != '"')
            i++;
        if (i >= end)
            break; // Exit if no more keys

        i++; // Move past opening '"'

        std::string key;
        // Extract key (with bounds check)
        while (i < end && s[i] != '"')
        {
            key += s[i];
            i++;
        }
        if (i >= end)
            throw std::runtime_error("Unterminated key");
        i++; // Move past closing '"'

        // Find colon
        while (i < end && s[i] != ':')
            i++;
        if (i >= end)
            throw std::runtime_error("Expected ':' after key");
        i++; // Skip ':'

        // Skip whitespace after colon
        while (i < end && isWhiteSpace(s[i]))
            i++;
        if (i >= end)
            throw std::runtime_error("Expected value after ':'");

        // Parse value
        std::string value;
        if (s[i] == '{')
        {
            ans[key] = parseObject(s, i, bracePair[i], bracePair);
            i = bracePair[i] + 1;
        }
        else if (s[i] == '[')
        {
            ans[key] = parseArray(s, i, bracePair[i], bracePair);
            i = bracePair[i] + 1;
        }
        else
        {
            while (i < end && s[i] != ',' && s[i] != '}')
            {
                value += s[i];
                i++;
            }
            ans[key] = getValue(value);
        }

        // Skip comma
        if (i < end && s[i] == ',')
            i++;
    }
    return ans;
}

// Example fix for parseArray to handle nested elements:
JSONNode parseArray(const std::string &s, int start, int end, std::unordered_map<int, int> &bracePair)
{
    int i = start + 1; // Skip '['
    JSONNode ans(JSONType::ARRAY);

    while (i < end)
    {
        // Skip whitespace
        while (i < end && isWhiteSpace(s[i]))
            i++;
        if (i >= end)
            break;

        // Handle empty elements (just skip them)
        if (s[i] == ',') {
            i++;
            continue;
        }

        if (s[i] == '{' || s[i] == '[')
        {
            int closingIndex = bracePair[i];
            if (s[i] == '{')
            {
                ans.appendArray(parseObject(s, i, closingIndex, bracePair));
            }
            else
            {
                ans.appendArray(parseArray(s, i, closingIndex, bracePair));
            }
            i = closingIndex + 1;
        }
        else
        {
            int valueStart = i;
            while (i < end && s[i] != ',')
                i++;
            std::string valueStr = s.substr(valueStart, i - valueStart);
            JSONNode value = getValue(valueStr);
            if (!value.isNULL()) {  // Only append non-null values
                ans.appendArray(value);
            }
            i++; // Skip ','
        }

        // Skip whitespace after value
        while (i < end && isWhiteSpace(s[i]))
            i++;
    }
    return ans;
}

std::string JSONNode::stringify(const JSONNode &node)
{
    switch (node.d_type)
    {
    case JSONType::BOOL:
        return node.d_value.d_bool ? "true" : "false";

    case JSONType::NULLT:
        return "null";

    case JSONType::NUMBER:
        return std::to_string(node.d_value.d_number);

    case JSONType::STRING:
    {
        std::string ans = "\"";
        ans += node.d_value.d_string;
        ans += '"';
        return ans;
    }

    case JSONType::ARRAY:
    {
        std::string ans = "[";
        for (auto v : node.d_array)
        {
            ans += stringify(v); // recusively stringify again
            ans += ',';
        }
        ans[ans.length() - 1] = ']'; // add last charactor as ]
        return ans;
    }

    case JSONType::OBJECT:
    {
        std::string ans = "{";
        for (auto &k : node.d_data)
        {
            ans += '"';
            ans += k.first;
            ans += '"';
            ans += ':';
            ans += stringify(k.second); // recursively stringify again
            ans += ',';
        }
        ans[ans.length() - 1] = '}'; // add last charactor as }
        return ans;
    }
    }
}