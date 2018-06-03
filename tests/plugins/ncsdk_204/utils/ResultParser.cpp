#include "ResultParser.h"
#include "Mutex.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef _WIN32
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#else
#include <Windows.h>
#endif


#ifndef CHECK
#define CHECK(VAL) if(!(VAL)) {printf("Check error, lineno = %d\n", __LINE__); std::exit(EXIT_FAILURE);}
#endif

typedef struct {
    int32_t nSide;
    int32_t n; //
    int32_t nClss;
    int32_t nSqrt;
    float *output;
}LastLayer;

typedef struct {
    float x;
    float y;
    float w;
    float h;
}RectFLT;

typedef struct {
    int32_t nIdx;
    int32_t nCls;
    float **fProbs;
} BRectSortable;

typedef struct tagLabelScore
{
    float score;
    std::string label;
}LabelScore;

const char* voc_names[] = { "aeroplane", "bicycle", "bird", "boat", "bottle", "bus", "car", "cat", "chair", "cow",
        "diningtable", "dog", "horse", "motorbike", "person", "pottedplant", "sheep", "sofa", "train", "tvmonitor" };
const char* barrier_names[] = { "minibus", "minitruck", "car", "mediumbus", "mpv", "suv", "largetruck", "largebus",
        "other" };

bool answerFileInitilized = false;
Mutex fileInit;
void getDetectionBoxes(LastLayer l, int32_t w, int32_t h, float thresh, float **probs, RectFLT *boxes,
        int32_t onlyObjectness)
{
    float top2Thr = thresh * 4 / 3;    // Top2 score threshold

    int32_t idx, m, n;
    float *predictions = l.output;
    int32_t count = l.nSide * l.nSide;
    for (idx = 0; idx < count; idx++) {
        int32_t row = idx / l.nSide;
        int32_t col = idx % l.nSide;

        for (n = 0; n < l.n; ++n) {
            int32_t index = idx * l.n + n;
            int32_t pIndex = l.nSide * l.nSide * l.nClss + idx * l.n + n;
            float scale = predictions[pIndex];
            int32_t box_index = l.nSide * l.nSide * (l.nClss + l.n) + (idx * l.n + n) * 4;
            boxes[index].x = (predictions[box_index + 0] + col) / l.nSide * w;
            boxes[index].y = (predictions[box_index + 1] + row) / l.nSide * h;
            boxes[index].w = pow(predictions[box_index + 2], (l.nSqrt ? 2 : 1)) * w;
            boxes[index].h = pow(predictions[box_index + 3], (l.nSqrt ? 2 : 1)) * h;

            float ascore[2] = { 0 };
            int32_t bHave = 0;
            int32_t maxscoreid = 0;

            for (m = 0; m < l.nClss; ++m) {
                int32_t class_index = idx * l.nClss;
                float prob = scale * predictions[class_index + m];
                probs[index][m] = prob > thresh ? prob : 0;

                // new modify
                {
                    if (probs[index][m] > thresh) {
                        bHave = 1;
                    }

                    // record top 2 score!
                    if (prob > ascore[0]) {
                        maxscoreid = m;
                        ascore[1] = ascore[0];
                        ascore[0] = prob;
                    } else if (prob > ascore[1]) {
                        ascore[1] = prob;
                    }
                }
            }
            if (onlyObjectness) {
                probs[index][0] = scale;
            }

            // new modify
            if (!bHave) {
                if (ascore[0] + ascore[1] > top2Thr) {
                    probs[index][maxscoreid] = ascore[0] + ascore[1];
                }
            }
        }
    }
}

int32_t nmsComparator(const void *pa, const void *pb)
{
    BRectSortable a = *(BRectSortable *) pa;
    BRectSortable b = *(BRectSortable *) pb;
    float diff = a.fProbs[a.nIdx][b.nCls] - b.fProbs[b.nIdx][b.nCls];
    if (diff < 0) {
        return 1;
    } else if (diff > 0) {
        return -1;
    }
    return 0;
}

float overLap(float x1, float y1, float x2, float y2)
{
    float t1, t2, t3, t4;

    float halfY1 = y1 / 2.0f;
    float halfY2 = y2 / 2.0f;

    t1 = x1 - halfY1;
    t2 = x2 - halfY2;
    t3 = x1 + halfY1;
    t4 = x2 + halfY2;

    return std::min(t3, t4) - std::max(t1, t2);
}

float overLapArea(RectFLT a, RectFLT b)
{
    float width = overLap(a.x, a.w, b.x, b.w);
    float height = overLap(a.y, a.h, b.y, b.h);

    if (width < 0 || height < 0) {
        return 0;
    }

    return width * height;
}

float boxUnion(RectFLT a, RectFLT b)
{
    float i = overLapArea(a, b);
    float u = a.w * a.h + b.w * b.h - i;
    return u;
}

float box_iou(RectFLT a, RectFLT b)
{
    return overLapArea(a, b) / boxUnion(a, b);
}

void nmsSort(RectFLT *boxes, float **probs, int32_t total, int32_t classes, float thresh)
{
    int32_t idx, m, n;
    BRectSortable *s = (BRectSortable*) calloc(total, sizeof(BRectSortable));
    if (NULL == s) {
        printf("Can't calloc!");
        std::exit(EXIT_FAILURE);
    }

    for (idx = 0; idx < total; ++idx) {
        s[idx].nIdx = idx;
        s[idx].nCls = 0;
        s[idx].fProbs = probs;
    }

    for (n = 0; n < classes; ++n) {
        for (idx = 0; idx < total; ++idx) {
            s[idx].nCls = n;
        }
        qsort(s, total, sizeof(BRectSortable), nmsComparator);

        for (idx = 0; idx < total; ++idx) {
            if (probs[s[idx].nIdx][n] == 0) {
                continue;
            }
            RectFLT a = boxes[s[idx].nIdx];
            for (m = idx + 1; m < total; ++m) {
                RectFLT b = boxes[s[m].nIdx];
                if (box_iou(a, b) > thresh) {
                    probs[s[m].nIdx][n] = 0;
                }
            }
        }
    }
    if (s) {
        free(s);
    }
}

int32_t maxIndex(float *fltA, int32_t n)
{
    if (n <= 0)
        return -1;
    int32_t i, max_i = 0;
    float max = fltA[0];
    for (i = 1; i < n; ++i) {
        if (fltA[i] > max) {
            max = fltA[i];
            max_i = i;
        }
    }
    return max_i;
}

void getOutRt(int32_t num, float thresh, RectFLT *boxes, float **probs, const char** labels, int32_t classes,
        int32_t srcImgW, int32_t srcImgH, std::vector<MyRect> & vecOutRt, std::vector<LabelScore>& vecLabel)
{
    int32_t idx;
    int32_t origin_cols = srcImgW;
    int32_t origin_rows = srcImgH;

    for (idx = 0; idx < num; ++idx) {
        int32_t nclass = maxIndex(probs[idx], classes);
        float prob = probs[idx][nclass];
        // printf("nclass=%d,thresh=%f, prob=%f, %f, %f, %f, %f\n",nclass,thresh, prob, boxes[i].x, boxes[i].y, boxes[i].w, boxes[i].h);
        if (prob > thresh) {
#ifdef _DEBUG
            //printf("class is:%s\n", labels[nclass]);
#endif
            LabelScore labelScore;
            labelScore.label = std::string(labels[nclass]);
            labelScore.score = prob;
            vecLabel.push_back(labelScore);

            RectFLT b = boxes[idx];

            int32_t left = (b.x - b.w / 2.) * origin_cols;
            int32_t right = (b.x + b.w / 2.) * origin_cols;
            int32_t top = (b.y - b.h / 2.) * origin_rows;
            int32_t bot = (b.y + b.h / 2.) * origin_rows;

            if (left < 0) {
                left = 0;
            }
            if (right > origin_cols - 1) {
                right = origin_cols - 1;
            }
            if (top < 0) {
                top = 0;
            }
            if (bot > origin_rows - 1) {
                bot = origin_rows - 1;
            }

#ifdef _DEBUG
            // printf("left=%d, right=%d, top=%d,bottom=%d\n", left, right, top, bot);
#endif
            MyRect myRt;
            myRt.x = left; myRt.y = top;
            myRt.w = right - left + 1;
            myRt.h = bot - top + 1;
            vecOutRt.push_back(myRt);
        }
    }
}

int32_t find_max(float * pf, int32_t cnt)
{
    int32_t id = 0, k;
    for (k = 1; k < cnt; ++k)
        if (pf[id] < pf[k])
            id = k;
    return id;
}
/*
static void nmsAll(RectFLT *boxes, float **probs, int32_t total, int32_t classes, float thresh)
{
    int32_t idx, m, n;

    for (idx = 0; idx < total; ++idx) {
        int32_t any = 0;
        for (n = 0; n < classes; ++n)
            any = any || (probs[idx][n] > 0);
        if (!any) {
            continue;
        }

        int32_t idi = find_max(probs[idx], classes);
        for (m = idx + 1; m < total; ++m) {
            float intersec = overLapArea(boxes[idx], boxes[m]);
            float boxarea_i = boxes[idx].w * boxes[idx].h;
            float boxarea_j = boxes[m].w * boxes[m].h;

            //printf(" >>>>>>>>>> %d, %d,  iou=%f,  intersec=%f  boxarea_i=%f  boxarea_j=%f \n", i, j, iou, intersec, boxarea_i, boxarea_j);

            if (intersec / boxarea_i > 0.6f || intersec / boxarea_j > 0.6f) {
                int32_t idj = find_max(probs[m], classes);

                if (probs[idx][idi] < probs[m][idj]) {
                    //printf("         >>>>>>>>>> i=%d      is erased\n", i);
                    memset(probs[idx], 0, sizeof(float) * classes);
                    break;
                } else {
                    //erase j continue
                    //printf("         >>>>>>>>>>    j=%d   is erased\n", j);
                    memset(probs[m], 0, sizeof(float) * classes);
                }
            }
        }
    }
}
*/

class CYoloDetectLayer
{
public:
    CYoloDetectLayer()
    {
        if (_clsNum == 9) {
            _ppLabelNames = (const char**) barrier_names;
        } else {
            _ppLabelNames = (const char**) voc_names;
        }
    }

    /**
     * @brief Copy constructor
     */
    CYoloDetectLayer(const CYoloDetectLayer& yoloDetectLayer) = delete;
    CYoloDetectLayer& operator=(const CYoloDetectLayer& yoloDetectLayer) = delete;
    ~CYoloDetectLayer()
    {
        if (_pfRslt) {
        free(_pfRslt);
        _pfRslt = nullptr;
        }
        if (_pRtBoxes) {
            delete[] _pRtBoxes;
            _pRtBoxes = nullptr;
        }
        if (_ppProbs) {
            for (int32_t j = 0; j < _lastLayout.nSide * _lastLayout.nSide * _lastLayout.n; ++j) {
                free(_ppProbs[j]);
                _ppProbs[j] = nullptr;
            }
            free(_ppProbs);
            _ppProbs = nullptr;
        }
    }

    /**
    *brief@ Parse vehicle position based on the HDDL output buffer.
    *param@ pLastLayer: the HDDL last layer output buffer.
    *param@ size: the HDDL last layer output buffer size, unit byte.
    *param@ vecOutRt: output parse result.
    */
    void detectLayer(void* pLastLayer, int32_t size/*byte num*/, int32_t precision,
        std::vector<MyRect>& vecOutRt,
        std::vector<LabelScore>& vecLabel,
        int32_t width, int32_t height)

    {
        //int32_t arrsz = size / sizeof(float);
        _lastLayout.nSide = _side;
        _lastLayout.n = _n;
        _lastLayout.nClss = _clsNum;
        _lastLayout.nSqrt = 1;     // ??
        _lastLayout.output = static_cast<float*>(pLastLayer);

        if (!_pRtBoxes) {
            int32_t bsize = sizeof(RectFLT);
            int32_t bnum = _lastLayout.nSide * _lastLayout.nSide * _lastLayout.n;
            _pRtBoxes = (RectFLT*) calloc(bnum, bsize);
        }

        if (!_ppProbs) {
            _ppProbs = (float**) calloc(_lastLayout.nSide * _lastLayout.nSide * _lastLayout.n, sizeof(float *));
            CHECK(_ppProbs);
            for (int32_t j = 0; j < _lastLayout.nSide * _lastLayout.nSide * _lastLayout.n; ++j) {
                _ppProbs[j] = (float*) calloc(_lastLayout.nClss, sizeof(float));
                CHECK(_ppProbs[j]);
            }
        }

        getDetectionBoxes(_lastLayout, 1, 1, _thresh, _ppProbs, _pRtBoxes, 0/*onlyObjectness*/);

        if (_fNMS) {
    #if 1    //Different label, do not merge.
            nmsSort(_pRtBoxes, _ppProbs, _lastLayout.nSide * _lastLayout.nSide * _lastLayout.n, _lastLayout.nClss, _fNMS);
    #else    //Different label, still merge.
            nmsAll(_pRtBoxes, _ppProbs, _lastLayout.nSide * _lastLayout.nSide * _lastLayout.n, _lastLayout.nClss, _fNMS);
    #endif
        }

        getOutRt(_lastLayout.nSide * _lastLayout.nSide * _lastLayout.n, _thresh, _pRtBoxes, _ppProbs, _ppLabelNames,
                _lastLayout.nClss, width, height, vecOutRt, vecLabel);
    }

private:
    const char ** _ppLabelNames;

    float *_pfRslt = nullptr;
    int32_t _nBufSize = 0;

    float _fNMS = 0.4;
    float _thresh = 0.200000003;
    LastLayer _lastLayout;
    RectFLT *_pRtBoxes = nullptr;
    float **_ppProbs = nullptr;

    int32_t _clsNum = 9;
    int32_t _side = 7;
    int32_t _n = 2;
};

/**
 * @brief parse parse_classification result
 * @param outputBuf.
 * @param dataSize : array size.
 * @param label : label name list[default null].
 * @param vecResult : output result[score, index, label name, output roi].
 */
void parse_tiny_yolo_v1(float* outputBuf, const int& dataSize,
        std::vector<std::tuple<float, int, std::string, MyRect> >& vecResult)
{
    std::vector<MyRect> vecOutRt;
    std::vector<LabelScore> vecLabel;
    CYoloDetectLayer yoloDetectLay;
    yoloDetectLay.detectLayer(outputBuf, dataSize * sizeof(float), sizeof(float), vecOutRt, vecLabel,
                448, 448);
    for(size_t i = 0; i < vecOutRt.size(); i++){
        vecResult.push_back(std::make_tuple(vecLabel[i].score, 0, vecLabel[i].label, vecOutRt[i]));
    }
}

int  compare_yolo(float* outputBuf, const int& dataSize, std::string answerFile, int resultIndex, bool ifPrint) {
    std::vector<std::tuple<float, int, std::string, MyRect> > vecResult;
    std::vector<std::tuple<float, int, std::string, MyRect> > vecAnswer;
    int ret = 0;
    /******parse result *******/
    /**************************/
    parse_tiny_yolo_v1(outputBuf, dataSize/sizeof(float), vecResult);
    //printf("vec result size = %d\n", vecResult.size());

    /******parse answer *******/
    /**************************/
    if (answerFile.length()) {
        if (!answerFileInitilized) {
            answerFileInitilized = true;

            char buf[100];
            std::ifstream rf(answerFile);
            if (rf.is_open()) {
                while(rf.getline(buf, sizeof(buf)) && !rf.eof()) {
                    int fileIndex;
                    MyRect ans;

                    std::string ss = buf;
                    sscanf(ss.c_str(), "%d_[%d,%d,%d,%d]", &fileIndex, &ans.x, &ans.y, &ans.w, &ans.h);
                    //printf("ans = %d, %d, %d, %d\n", ans.x, ans.y, ans.w, ans.h);
                    yoloAnswer.push_back(std::make_tuple(fileIndex, 0, 0, std::string(""), ans));
                }
            }
        }
        for (auto i : yoloAnswer) {
            if (std::get<0>(i) == resultIndex) {
                vecAnswer.push_back(std::make_tuple(std::get<1>(i), std::get<2>(i), std::get<3>(i), std::get<4>(i)));
            }
        }
    }

    /******compare result******/
    /**************************/
    if (answerFile.length()) {
        if (vecAnswer.size() != vecResult.size()) {
            std::cout << "Compare failed, result is rejected." << std::endl;
            for (auto i: vecAnswer) {
                int x = std::get<3>(i).x;
                int y = std::get<3>(i).y;
                int w = std::get<3>(i).w;
                int h = std::get<3>(i).h;
                std::cout << " - answer : " << std::get<2>(i) << ", " << x << ", " << y << ", " << w << ", " << h << ", " << std::endl;
            }

            for (auto i: vecResult) {
                int x = std::get<3>(i).x;
                int y = std::get<3>(i).y;
                int w = std::get<3>(i).w;
                int h = std::get<3>(i).h;
                std::cout << " - result : " << std::get<2>(i) << ", " << x << ", " << y << ", " << w << ", " << h << ", " << std::endl;
            }
            ret = -1;
        } else {
            for (unsigned int i = 0; i < vecAnswer.size(); i++) {
                int xa = std::get<3>(vecAnswer[i]).x;
                int ya = std::get<3>(vecAnswer[i]).y;
                int wa = std::get<3>(vecAnswer[i]).w;
                int ha = std::get<3>(vecAnswer[i]).h;

                int xr = std::get<3>(vecResult[i]).x;
                int yr = std::get<3>(vecResult[i]).y;
                int wr = std::get<3>(vecResult[i]).w;
                int hr = std::get<3>(vecResult[i]).h;

                int thr = 8;
                if (abs(xa - xr) > thr ||
                    abs(ya - yr) > thr ||
                    abs(wa - wr) > thr ||
                    abs(ha - hr) > thr) {
                    if(!ret) {
                        ret = -1;
                        std::cout << "Compare failed, result is rejected." << std::endl;
                    }

                    std::cout << " - answer : " << std::get<2>(vecAnswer[i]) << ", " << xa << ", " << ya << ", " << wa << ", " << ha << ", " << std::endl;
                    std::cout << " - result : " << std::get<2>(vecResult[i]) << ", " << xr << ", " << yr << ", " << wr << ", " << hr << ", " << std::endl;
                } else {
                    if (ifPrint) {
                        std::cout << " - result : " << std::get<2>(vecResult[i]) << ", " << xr << ", " << yr << ", " << wr << ", " << hr << ", " << std::endl;
                    }
                }
            }
        }
    } else {
        if (ifPrint) {
            for (auto i : vecResult) {
                int xr = std::get<3>(i).x;
                int yr = std::get<3>(i).y;
                int wr = std::get<3>(i).w;
                int hr = std::get<3>(i).h;

                std::cout << " - result : " << std::get<2>(i) << ", " << xr << ", " << yr << ", " << wr << ", " << hr << ", " << std::endl;
            }
        }
    }

    return ret;
}

void parse_classification(float* outputBuf, const int& dataSize, std::string* label, const int& outTopNum,
        std::vector<std::tuple<float, int, std::string> >& vecResult)
{
    const float *pBuf = outputBuf;
    int sz = dataSize;
    if (!pBuf){
        std::cout << "Unsupport out data type " << __LINE__ << " : " << __FILE__ << std::endl;
        std::exit(EXIT_FAILURE);
    }

    std::vector<int> index(sz);
    std::iota(std::begin(index), std::end(index), 0);

    // Get top N results
    int outNum = std::min(outTopNum, sz);
    std::partial_sort(index.begin(), index.begin() + outNum, index.end(), [&pBuf](const int& a, const int& b) {
        return pBuf[a] > pBuf[b];
    });

    for (int i = 0; i < outNum; i++) {
        if (label) {
            vecResult.push_back(std::make_tuple(pBuf[index[i]], index[i], std::string(label[index[i]])));
        } else {
            vecResult.push_back(std::make_tuple(pBuf[index[i]], index[i], std::string()));
        }
    }
}

int  compare_google(float* outputBuf, const int& dataSize, std::string answerFile, int resultIndex, bool ifPrint)
{
    std::vector<std::tuple<float, int, std::string> > vecResult;

    int num = 2;
    int ret = 0;
    /*****parse result****/
    /*********************/
    parse_classification(outputBuf, dataSize/sizeof(float)/*float 32 = 4 bytes*/, label_googlenetv1, num, vecResult);

    /*****parse answer****/
    /*********************/
    if (answerFile.length()) {
        fileInit.lock();
        if (!answerFileInitilized) {
            answerFileInitilized = true;
            char buf[100];

            std::ifstream rf(answerFile);
            if (rf.is_open()) {
                while(rf.getline(buf, sizeof(buf)) && !rf.eof()) {
                    int fileIndex;
                    int index;
                    float confidence;
                    std::string ss = buf;
                    std::string rest = ss.substr(0, ss.rfind("_"));
                    std::string carName = ss.substr(ss.rfind("_")+1);
                    sscanf(rest.c_str(), "%d_%d_%f", &fileIndex, &index, &confidence);
                    printf("answer confidence = %f, index = %d, carName = %s\n", confidence, index, carName.c_str());
                    googleAnswer.push_back(std::make_tuple(confidence, index, carName));
                }
            }
        }
        fileInit.unlock();
    }
    /**compare answer**/
    if (answerFile.length() && googleAnswer.size()) {
        if (std::get<1>(vecResult[0]) != std::get<1>(googleAnswer[resultIndex-1])) {
                if (!ret) {
                    std::cout << "Compare failed, result is rejected, input file is " << resultIndex << ".dat" << std::endl;
                    ret = -1;
                }
                std::cout << " -result : " << std::get<1>(vecResult[0]) << " confidence : " << std::get<0>(vecResult[0]) << " class name: " << std::get<2>(vecResult[0]) << std::endl;
                std::cout << " -answer : " << std::get<1>(googleAnswer[resultIndex-1]) << " confidence : " << std::get<0>(googleAnswer[resultIndex-1]) << " class name: " << std::get<2>(googleAnswer[resultIndex-1]) << std::endl;
            } else {
                if (ifPrint) {
                    std::cout << " Result accepted " << resultIndex << ".dat -result : confidence : " << std::get<0>(vecResult[0]) << " index:" << std::get<1>(vecResult[0]) << " class name: " << label_googlenetv1[std::get<1>(vecResult[0])] << std::endl;
                }
            }
        } else {
            for (auto i : vecResult){
                if (ifPrint) {
                    std::cout << " Result accepted " << resultIndex << ".dat -result : confidence : " << std::get<0>(i) << " index:" << std::get<1>(i) << " class name: " << label_googlenetv1[std::get<1>(i)] << std::endl;
                }
            }
    }

    return ret;
}

bool isFile(const char* cate_dir)
{
    struct stat buffer;
    stat(cate_dir, &buffer);

    if ((buffer.st_mode & S_IFREG )!= 0) {
        return true;
    }
    return false;
}

int  parseResult(float* outputBuf, const int& dataSize, std::string answerFile, int resultIndex, bool ifPrint, GraphType type)
{
    if (!outputBuf) {
        printf("Error: Null outputBuf pointer\n");
    }

    switch(type) {
        case GOOGLE_NET: return compare_google(outputBuf, dataSize, answerFile, resultIndex, ifPrint);
        case YOLO_NET:   return compare_yolo(outputBuf, dataSize, answerFile, resultIndex, ifPrint);
        default : printf("Unknown type %d, break.", type);break;
    }

    return -1;
}

std::string getAnswerFile(const char* answerFolder, const char* pictureFile)
{
    if (isFile(answerFolder)) {
        return std::string(answerFolder);
    }
    std::string picFile(pictureFile);
    std::string pictureName = picFile.substr(picFile.rfind("/") + 1, (picFile.rfind(".") - picFile.rfind("/")));
    std::string answerFile = pictureName.substr(0, pictureName.find("_"));
    std::string ret = std::string(answerFolder) + "/" + answerFile + ".txt";

    return ret;
}

int  getAnswerIndex(const char* pictureFile)
{
    std::string picFile(pictureFile);
    std::string pictureName = picFile.substr(picFile.rfind("/") + 1, (picFile.find("_", picFile.rfind("/") + 1)));
    return std::stoi(pictureName);
}
