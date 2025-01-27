// #include <sstream>
#include <string>
#include <iostream>


int main(void)
{    
    std::string body = "----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                       "Content-Disposition: form-data; name=\"field1\"\r\n\r\n"
                       "value1\r\n"
                       "----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                       "Content-Disposition: form-data; name=\"file1\"; filename=\"file.txt\"\r\n"
                       "Content-Type: text/plain\r\n\r\n"
                       "This is the content of the file.\r\n"
                       "----WebKitFormBoundary7MA4YWxkTrZu0gW--";
    std::string boundary = "----WebKitFormBoundary7MA4YWxkTrZu0gW";
    std::vector<std::string> myVec;
    int start = 0;
    int end = 0;
    int i = 0;
    int l = 0;
    while (i < body.size())
    {
        if (l == 1)
            break ;
        if (body.find(boundary, start) != std::string::npos) //go over the boundary
        {
            start = i + boundary.size() + 2;
            // std::cout << "boundary size: " << boundary.size() << "--" << std::endl;
            // std::cout << body.substr(boundary.size() + 2) << "##" << std::endl;
            // std::cout << "start index " << start << std::endl;
            // std::string sub = body.substr(start);
            // std::cout << sub << "##" << std::endl;
            if (body.find(boundary, start) != std::string::npos)
            {
                end = body.find(boundary, start);
            }
            std::string sub = body.substr(start, end - start);
            std::cout << sub << "##" << std::endl;
            myVec.push_back(sub);
            sub.clear();
        }
        else
        {
            if (body.find(boundary + "--") != std::string::npos)
                break;
        }
        i = end;
        l++;
    }

}