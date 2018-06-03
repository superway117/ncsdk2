#ifndef _PARSE_NETWORK_H_
#define _PARSE_NETWORK_H_

#include <vector>
#include <tuple>
#include <string>
#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <cmath>

//#include "CommandLineParser.h"

typedef enum {
    GOOGLE_NET,
    YOLO_NET,
    UNKNOWN_NET
} GraphType;

// label: google net v1, classify vehicle types(1062 types)
static std::string label_googlenetv1[] =
{
#include "label_googlenetv1.txt"
};

// label: vggd, classify imagenet(1000 types)
static std::string label_vggd[] =
{
#include "label_vggd.txt"
};

/**
 * @brief parse parse_classification result
 * @param outputBuf.
 * @param dataSize : array size.
 * @param label : label name list[default null].
 * @param outTopNum : output top.
 * @param vecResult : output result[score, index, label name(default null, if(label) have value)].
 * @show result sample:
    std::vector<std::tuple<float, int, std::string> > vecResult;
    for (auto i : vecResult) {
        std::cout << "score = " << std::get<0>(i) << std::endl;
        std::cout << "index = " << std::get<1>(i) << std::endl;
        std::cout << "label = " << std::get<2>(i) << std::endl;
    }
 */
void parse_classification(float* outputBuf, const int& dataSize, std::string* label, const int& outTopNum,
        std::vector<std::tuple<float, int, std::string> >& vecResult);

/**
 * @brief parse parse_classification result
 * @param outputBuf.
 * @param dataSize : array size.
 * @param label : label name list[default null].
 * @param vecResult : output result[score, index, label name, output roi].
 */
typedef struct tagMyRect
{
    int x;
    int y;
    int w;
    int h;
}MyRect;

static std::vector<std::tuple<int, float, int, std::string, MyRect> > yoloAnswer;
static std::vector<std::tuple<float, int, std::string> > googleAnswer;
void parse_tiny_yolo_v1(float* outputBuf, const int& dataSize,
        std::vector<std::tuple<float, int, std::string, MyRect> >& vecResult);

int  compare_yolo(float* outputBuf, const int& dataSize, std::string answerFile, int resultIndex, bool ifPrint);
int  compare_google(float* outputBuf, const int& dataSize, std::string answerFile, int resultIndex, bool ifPrint);

int  parseResult(float* outputBuf, const int& dataSize, std::string answerFile, int resultIndex, bool ifPrint, GraphType type);

std::string getAnswerFile(const char* answerFolder, const char* pictureFile);
int  getAnswerIndex(const char* pictureFile);
#endif /* _PARSE_NETWORK_H_ */