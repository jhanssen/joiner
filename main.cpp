#include <dirent.h>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

bool append(FILE* f, const std::string& file)
{
    enum { BlockSize = 1024 * 1024 * 2 };
    char block[BlockSize];
    FILE* in = fopen(file.c_str(), "r");
    if (!in)
        return false;
    while (!feof(in)) {
        size_t r = fread(block, 1, BlockSize, in);
        if (!r) {
            fclose(in);
            return true;
        }
        size_t w = fwrite(block, 1, r, f);
        if (w != r) {
            fclose(in);
            return false;
        }
    }
    fclose(in);
    return true;
}

bool join(const std::string& out, const std::vector<std::string>& files)
{
    FILE* f = fopen(out.c_str(), "w");
    if (!f)
        return false;
    const size_t n = files.size();
    for (size_t i = 0; i < n; ++i) {
        const std::string file = files[i];
        if (!append(f, file)) {
            fclose(f);
            return false;
        }
    }
    fclose(f);

    return true;
}

std::string padext(int num, int size)
{
    // awful
    char buf1[100], buf2[100];
    const int tot = snprintf(buf1, sizeof(buf1), "%d", num);
    if (tot >= 10)
        return std::string();
    if (tot > size)
        size = tot;
    memset(buf2, '0', sizeof(buf2) - 1);
    buf2[size] = '\0';
    strcpy(buf2 + size - tot, buf1);

    return std::string(buf2);
}

bool findfiles(const std::string& base, std::vector<std::string>& files, int extsz)
{
    if (extsz >= 10)
        return false;
    struct stat st;
    for (int i = 1;; ++i) {
        const std::string ext = padext(i, extsz);
        if (stat((base + "." + ext).c_str(), &st) != 0)
            break;
        if (!S_ISREG(st.st_mode))
            break;
        files.push_back(base + "." + ext);
    }
    return true;
}

int main(int, char**)
{
    DIR* dir = opendir(".");
    if (!dir)
        return 1;
    dirent* ent;
    while ((ent = readdir(dir)) != 0) {
        const std::string f(ent->d_name);
        const int dot = f.rfind('.');
        if (dot == std::string::npos)
            continue;
        char* end;
        const long int num = strtol(f.c_str() + dot + 1, &end, 10);
        if (!end || *end != '\0' || num != 1)
            continue;
        const std::string base = f.substr(0, dot).c_str();
        printf("joining %s\n", base.c_str());
        std::vector<std::string> files;
        if (!findfiles(base, files, end - (f.c_str() + dot + 1)))
            continue;
        if (files.empty())
            continue;
        join(base, files);
        printf("joined %s\n", base.c_str());
    }
    closedir(dir);
    return 0;
}
