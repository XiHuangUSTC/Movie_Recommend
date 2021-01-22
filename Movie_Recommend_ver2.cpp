#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <vector>
#include <algorithm>
#include <time.h>

#define FILEPATH_TRAIN "training.dat"
#define FILEPATH_TEST "testing.dat"
#define FILEPATH_OUTPUT "output.txt"
#define K_VALUE 5    //K-近邻的K值
#define SIM_SIZE 2000    //相似度缓存区大小

using namespace std;

bool Cmp_By_Sim(pair<int, float> pair1, pair<int, float> pair2){
    return pair1.second > pair2.second;
}

int main(){
    int i, j, k, count = 1, userID, movieID, score, userNum = 0, movieNum = 0, watchedNum, neighbourNum;
    float dot_product, vec1_Length, vec2_Length, predict_Score, simSum, vec1_Rank, vec2_Rank;
    char ch;
    string line;
    ifstream f_Train(FILEPATH_TRAIN);
    ifstream f_Test(FILEPATH_TEST);
    ofstream f_Output(FILEPATH_OUTPUT);
    map<int, int> map_User, map_Movie;    //key为ID，value为在矩阵中的下标
    pair<map<int, int>::iterator, bool> insert_Pair;
    map<int, int>::iterator iter_User, iter_Movie;
    srand(time(NULL));

    //统计用户数量和电影数量，制作用户map和电影map
    do{
        f_Train >> userID;
        f_Train >> ch;
        f_Train >> movieID;
        getline(f_Train, line);
        insert_Pair = map_User.insert(pair<int, int>(userID, userNum));
        if(insert_Pair.second == true)
            userNum++;
        insert_Pair = map_Movie.insert(pair<int, int>(movieID, movieNum));
        if(insert_Pair.second == true)
            movieNum++;
    } while (f_Train.eof() == false);
    f_Train.close();
    cout << "User_Map and Movie_Map...done" << endl;

    vector<map<int, float>> rank_User(userNum);    //用户给电影的打分(用户视角)
    vector<map<int, float>> rank_Movie(movieNum);  //用户给电影的打分(电影视角)
    vector<float> average(movieNum);      //电影的平均分
    map<int, float>::iterator iter_Rank, iter_Rank2;
    map<int, vector<float>> map_Sim;    //电影之间的相似度
    map<int, vector<float>>::iterator iter_Sim;
    vector<float> similar(movieNum);
    vector<pair<int, float>> watched_Movie;   //pair.first:下标，pair.second:相似度

    //读取用户给电影的打分
    f_Train.open(FILEPATH_TRAIN);
    do{
        f_Train >> userID;
        f_Train >> ch;
        f_Train >> movieID;
        f_Train >> ch;
        f_Train >> score;
        getline(f_Train, line);
        iter_User = map_User.find(userID);
        iter_Movie = map_Movie.find(movieID);
        rank_User[(*iter_User).second].insert(pair<int, float>((*iter_Movie).second, score));
        rank_Movie[(*iter_Movie).second].insert(pair<int, float>((*iter_User).second, score));
    } while (f_Train.eof() == false);
    cout << "rank...done" << endl;

    //去中心化
    for (i = 0; i < movieNum; i++){
        average[i] = 0;
        for (iter_Rank = rank_Movie[i].begin(); iter_Rank != rank_Movie[i].end(); iter_Rank++)
            average[i] = average[i] + (*iter_Rank).second;
        average[i] = average[i] / rank_Movie[i].size();
        for (j = 0; j < userNum; j++){
            iter_Rank = rank_User[j].find(i);
            if(iter_Rank != rank_User[j].end())
                (*iter_Rank).second = (*iter_Rank).second - average[i];
        }
        for (iter_Rank = rank_Movie[i].begin(); iter_Rank != rank_Movie[i].end(); iter_Rank++)
            (*iter_Rank).second = (*iter_Rank).second - average[i];
    }
    cout << "decentral...done" << endl;

    //预测测试集中的用户打分
    while(f_Test.eof() == false){
        f_Test >> userID;
        f_Test >> ch;
        f_Test >> movieID;
        getline(f_Test, line);
        iter_User = map_User.find(userID);
        iter_Movie = map_Movie.find(movieID);

        if(iter_User == map_User.end() && iter_Movie != map_Movie.end())    //出现新用户
            f_Output << average[(*iter_Movie).second] << endl;
        else if(iter_User != map_User.end() && iter_Movie == map_Movie.end()){   //出现新电影
            predict_Score = 0;
            for (iter_Rank = rank_User[(*iter_User).second].begin(); iter_Rank != rank_User[(*iter_User).second].end(); iter_Rank++)
                predict_Score = predict_Score + (*iter_Rank).second + average[(*iter_Rank).first];
            predict_Score = predict_Score / rank_User[(*iter_User).second].size();
            f_Output << predict_Score << endl;
        }
        else if(iter_User == map_User.end() && iter_Movie == map_Movie.end()){  //同时出现新电影和新用户
            predict_Score = 0;
            watchedNum = 0;
            for (i = 0; i < userNum; i++){
                for (iter_Rank = rank_User[i].begin(); iter_Rank != rank_User[i].end(); iter_Rank++)
                    predict_Score = predict_Score + (*iter_Rank).second;
                watchedNum = watchedNum + rank_User[i].size();
            }
            predict_Score = predict_Score / watchedNum;
            f_Output << predict_Score << endl;
        }
        else{
            iter_Rank = rank_User[(*iter_User).second].find((*iter_Movie).second);
            if(iter_Rank != rank_User[(*iter_User).second].end()){   //此用户已经给此电影评分了
                predict_Score = (*iter_Rank).second;
                f_Output << predict_Score << endl;
            }
            else{     //正常情况
                iter_Sim = map_Sim.find((*iter_Movie).second);
                if(iter_Sim == map_Sim.end()){      //缓存中没有此电影与其他电影的相似度
                    //计算相似度
                    for (i = 0; i < movieNum; i++){
                        dot_product = 0;
                        vec1_Length = 0;
                        vec2_Length = 0;
                        iter_Rank = rank_Movie[(*iter_Movie).second].begin();
                        iter_Rank2 = rank_Movie[i].begin();
                        for (j = 0; j < userNum; j++){
                            if((*iter_Rank).first == j){
                                vec1_Rank = (*iter_Rank).second;
                                iter_Rank++;
                            }
                            else
                                vec1_Rank = 0;
                            if((*iter_Rank2).first == j){
                                vec2_Rank = (*iter_Rank2).second;
                                iter_Rank2++;
                            }
                            else
                                vec2_Rank = 0;
                            dot_product = dot_product + vec1_Rank * vec2_Rank;
                            vec1_Length = vec1_Length + pow(vec1_Rank, 2);
                            vec2_Length = vec2_Length + pow(vec2_Rank, 2);
                        }
                        vec1_Length = sqrt(vec1_Length);
                        vec2_Length = sqrt(vec2_Length);
                        similar[i] = dot_product / (vec1_Length * vec2_Length);
                    }
                    if(map_Sim.size() < SIM_SIZE)    //缓存还有空位
                    {
                        map_Sim.insert(pair<int, vector<float>>((*iter_Movie).second, similar));
                    }
                    else{         //缓存已满
                        i = 0;
                        for (iter_Sim = map_Sim.begin(); i < rand() % SIM_SIZE; iter_Sim++)
                            i++;
                        map_Sim.erase(iter_Sim);
                        map_Sim.insert(pair<int, vector<float>>((*iter_Movie).second, similar));
                    }
                    iter_Sim = map_Sim.find((*iter_Movie).second);
                }
                //寻找K个最大相似度的电影
                vector<pair<int, float>>().swap(watched_Movie);
                for (iter_Rank = rank_User[(*iter_User).second].begin(); iter_Rank != rank_User[(*iter_User).second].end(); iter_Rank++)
                    watched_Movie.push_back(pair<int, float>((*iter_Rank).first, (*iter_Sim).second[(*iter_Rank).first]));
                nth_element(watched_Movie.begin(), watched_Movie.begin() + K_VALUE - 1, watched_Movie.end(), Cmp_By_Sim);
                //计算预测评分
                predict_Score = 0;
                simSum = 0;
                if(watched_Movie.size() < K_VALUE)
                    neighbourNum = watched_Movie.size();
                else
                    neighbourNum = K_VALUE;
                for (i = 0; i < neighbourNum; i++){
                    predict_Score = predict_Score + watched_Movie[i].second * rank_User[watched_Movie[i].first][(*iter_User).second];
                    simSum = simSum + watched_Movie[i].second;
                }
                predict_Score = predict_Score / simSum;
                f_Output << predict_Score << endl;
            }
        }
        cout << count << endl;
        count++;
    }

    f_Train.close();
    f_Test.close();
    f_Output.close();
    std::system("pause");
    return 0;
}