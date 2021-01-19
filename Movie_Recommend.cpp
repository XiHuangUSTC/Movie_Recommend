#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <vector>
#include <algorithm>

#define FILEPATH_TRAIN "training.dat"
#define FILEPATH_TEST "testing.dat"
#define FILEPATH_OUTPUT "output.txt"
#define K 5    //K-近邻的K值

using namespace std;

bool Cmp_By_Sim(pair<int, double> pair1, pair<int, double> pair2){
    return pair1.second > pair2.second;
}

int main(){
    int i, j, k, count = 1, userID, movieID, score, userNum = 0, movieNum = 0, watchedNum;
    double average, dot_product, vec1_Length, vec2_Length, predict_Score, simSum;
    char ch;
    string line;
    ifstream f_Train(FILEPATH_TRAIN);
    ifstream f_Test(FILEPATH_TEST);
    ofstream f_Output(FILEPATH_OUTPUT);
    map<int, int> map_User, map_Movie;    //key为ID，value为在矩阵中的下标
    pair<map<int, int>::iterator, bool> insert_Pair;
    map<int, int>::iterator iter_User, iter_Movie;

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
    cout << "制作用户map和电影map...完成" << endl;
    cout << movieNum << " " << userNum << endl;

    //读取用户给电影的打分
    int rank[movieNum][userNum];     //用户给电影打分矩阵
    double decentral_Rank[movieNum][userNum];   //去中心化后的rank
    double sim[movieNum][movieNum];  //电影之间相似度矩阵
    vector<pair<int, double>> watched_Movie;   //pair.first:在矩阵中的下标，pair.second:相似度
    f_Train.open(FILEPATH_TRAIN);

    for (i = 0; i < movieNum; i++)     //未打分的为-1
        for (j = 0; j < userNum; j++)
            rank[i][j] = -1;
    do{
        f_Train >> userID;
        f_Train >> ch;
        f_Train >> movieID;
        f_Train >> ch;
        f_Train >> score;
        getline(f_Train, line);
        iter_User = map_User.find(userID);
        iter_Movie = map_Movie.find(movieID);
        rank[(*iter_Movie).second][(*iter_User).second] = score;
    } while (f_Train.eof() == false);
    cout << "读取用户给电影的打分...完成" << endl;

    //去中心化
    for (i = 0; i < movieNum; i++){
        watchedNum = 0;
        average = 0;
        for (j = 0; j < userNum; j++)
            if(rank[i][j] != -1){
                watchedNum++;
                average = average + rank[i][j];
            }
        average = average / watchedNum;
        for (j = 0; j < userNum; j++){
            if(rank[i][j] == -1)
                decentral_Rank[i][j] = 0;
            else
                decentral_Rank[i][j] = rank[i][j] - average;
        }
    }
    cout << "去中心化...完成" << endl;

    //计算电影之间的相似度
    for (i = 0; i < movieNum; i++){
        for (j = i; j < movieNum; j++){
            dot_product = 0;
            vec1_Length = 0;
            vec2_Length = 0;
            for (k = 0; k < userNum; k++){
                dot_product = dot_product + decentral_Rank[i][k] * decentral_Rank[j][k];
                vec1_Length = vec1_Length + pow(decentral_Rank[i][k], 2);
                vec2_Length = vec2_Length + pow(decentral_Rank[j][k], 2);
            }
            vec1_Length = sqrt(vec1_Length);
            vec2_Length = sqrt(vec2_Length);
            sim[i][j] = sim[j][i] = dot_product / (vec1_Length * vec2_Length);
        }
    }
    cout << "计算相似度...完成" << endl;

    //预测测试集中的用户打分
    do{
        f_Test >> userID;
        f_Test >> ch;
        f_Test >> movieID;
        getline(f_Test, line);
        iter_User = map_User.find(userID);
        iter_Movie = map_Movie.find(movieID);
        
        if(iter_User == map_User.end() && iter_Movie != map_Movie.end()){   //出现新用户
            watchedNum = 0;
            predict_Score = 0;
            for (i = 0; i < userNum; i++)
                if(rank[(*iter_Movie).second][i] != -1){
                    watchedNum++;
                    predict_Score = predict_Score + rank[(*iter_Movie).second][i];
                }
            predict_Score = predict_Score / watchedNum;
            f_Output << predict_Score << endl;
        }
        else if(iter_User != map_User.end() && iter_Movie == map_Movie.end()){   //出现新电影
            watchedNum = 0;
            predict_Score = 0;
            for (i = 0; i < movieNum; i++)
                if(rank[i][(*iter_User).second] != -1){
                    watchedNum++;
                    predict_Score = predict_Score + rank[i][(*iter_User).second];
                }
            predict_Score = predict_Score / watchedNum;
            f_Output << predict_Score << endl;
        }
        else if(iter_User == map_User.end() && iter_Movie == map_Movie.end()){//同时出现新电影和新用户
            watchedNum = 0;
            predict_Score = 0;
            for (i = 0; i < movieNum; i++){
                for (j = 0; j < userNum; j++)
                    if(rank[i][j] != -1)
                        watchedNum++;
                        predict_Score = predict_Score + rank[i][j];
                    }
            predict_Score = predict_Score / watchedNum;
            f_Output << predict_Score << endl;
        }
        else{
            if(rank[(*iter_Movie).second][(*iter_User).second] != -1)    //此用户已经给此电影评分了
                f_Output << rank[(*iter_Movie).second][(*iter_User).second] << endl;
            else{       //正常情况
                //寻找K个最大相似度的电影
                watched_Movie.clear();
                for (i = 0; i < movieNum; i++)
                    if(rank[i][(*iter_User).second] != -1)
                        watched_Movie.push_back(pair<int, double>(i, sim[(*iter_Movie).second][i]));
                nth_element(watched_Movie.begin(), watched_Movie.begin() + K - 1, watched_Movie.end(), Cmp_By_Sim);
                //计算预测评分
                predict_Score = 0;
                simSum = 0;
                for (i = 0; i < K; i++){
                    predict_Score = predict_Score + watched_Movie[i].second * rank[watched_Movie[i].first][(*iter_User).second];
                    simSum = simSum + watched_Movie[i].second;
                }
                predict_Score = predict_Score / simSum;
                f_Output << predict_Score << endl;
            }
        }
        cout << count << endl;
        count++;
    } while (f_Test.eof() == false);

    f_Train.close();
    f_Test.close();
    f_Output.close();
    std::system("pause");
    return 0;
}