#include <iostream>
#include <stdlib.h>
#include "stdio.h"
#include <caffe/caffe.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "opencv2/imgproc/imgproc.hpp"
#include <math.h>
#include <functional>
#include <chrono>

using namespace std;
using namespace caffe;
using namespace cv;

#define RANDDOUBLE ((rand() % RAND_MAX) * 1.0 / RAND_MAX)
#define BK_GROUND -1.0f
#define FR_GROUND 1.0f

void renderRect(int imW, int imH, int rW, int rH, int cx, int cy, float rot, float* data) {
    float c = cos(rot);
    float s = sin(rot);
    for (int y = 0; y < imH; y++) {
        for (int x = 0; x < imW; x++) {
            int locx = (x - cx) * c - (y - cy) * s;
            int locy = (x - cx) * s + (y - cy) * c;
            data[x + imW * y] = (-rW / 2 < locx) && (locx < rW / 2) && (-rH / 2 < locy) && (locy < rH / 2) ? FR_GROUND : BK_GROUND;
        }
    }
}

void renderImage(int imW, int imH, int cx, int cy, float rot, float* data, const Mat& img) {
    warpAffine(img, Mat(imH, imW, CV_32F, data),
        (Mat_<float>(2, 3) << 1, 0, cx, 0, 1, cy) * // translate to desired location
        (Mat_<float>(3, 3) << cos(rot), -sin(rot), 0, sin(rot), cos(rot), 0, 0, 0, 1) * // rotate
        (Mat_<float>(3, 3) << 1, 0, -img.rows / 2, 0, 1, -img.cols / 2, 0, 0, 1), // center
        Size(imH, imW), INTER_LINEAR, BORDER_CONSTANT, Scalar(BK_GROUND));
}

void drawDat(const char* fname, int w, int h, float* dat, function<float(float)> f) {
    Mat m(h, w, CV_32F);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            m.at<float>(y, x) = f(dat[y * w + x]);
        }
    }
    imwrite(fname, m);
}

int main(int argc, char ** argv) {
    // Mat rm(100, 100, CV_32F);
    // renderRect(100, 100, 30, 30, 70, 50, 0.4, (float*)rm.data);
    // imwrite("test.png", rm);
    // return 0;
    Mat characterImg = imread("../character.png", CV_LOAD_IMAGE_GRAYSCALE);
    Mat character(50, 50, CV_32F);
    for (int j = 0; j < characterImg.cols; j++) {
        for (int i = 0; i < characterImg.rows; i++) {
            character.at<float>(i, j) = characterImg.at<unsigned char>(i, j) / -255.0 * (FR_GROUND - BK_GROUND) + FR_GROUND;
        }
    }

    // float* a = (float*)malloc(40000);
    // renderImage(100, 100, 20, 20, M_PI / 2, a, character);
    // drawDat("test.png", 100, 100, a, [](float x) { return x * 127 + 128; });return 0;

    srand(time(0));
    ::google::InitGoogleLogging(argv[0]);

    Caffe::set_mode(Caffe::GPU);

    SolverParameter solver_param;
    ReadSolverParamsFromTextFileOrDie("../lenet_solver.prototxt", &solver_param);

    std::shared_ptr<Solver<float> > solver;
    solver.reset(SolverRegistry<float>::CreateSolver(solver_param));

    if (argc > 1)
        solver->Restore(argv[1]);

    Net<float>* net = solver->net().get();

    const int BATCH_SIZE = net->input_blobs()[0]->shape(0);
    const int WIDTH = net->input_blobs()[0]->shape(2);
    const int HEIGHT = net->input_blobs()[0]->shape(3);
    const int NUM_LABELS = net->input_blobs()[1]->shape(1);

    const int MAX_MOVE = 5;//((NUM_LABELS / 2) - 1) / 2;
    cout << "Learning params: MAX_MOVE: " << MAX_MOVE << ", NUM_LABELS: " << NUM_LABELS << endl;

    int rectW = 50;
    int rectH = 50;

    // auto start = std::chrono::system_clock::now();
    // std::chrono::duration<double> totWasted(0);

    while (true) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            float* imgPtr = net->input_blobs()[0]->mutable_cpu_data() + i * WIDTH * HEIGHT * 2;
            float* labPtr = net->input_blobs()[1]->mutable_cpu_data() + i * NUM_LABELS;

            // Try generate random params:
            float rot;
            int randx;
            int randy;
            int randdx;
            int randdy;
            float drot;
            float locx;
            float locy;
            int movx;
            int movy;
            // check that center is 1
            // auto startC = std::chrono::system_clock::now();
            // int numTries = 0;
            do {
                // numTries++;
                rot = RANDDOUBLE * M_PI * 2;
                drot = RANDDOUBLE * M_PI / 9 - M_PI / 18;
                randx = rand() % 49 + 26;
                randy = rand() % 49 + 26;
                randdx = rand() % 11 - 5;
                randdy = rand() % 11 - 5;

                float c = cos(rot);
                float s = sin(rot);
                float cp = cos(rot + drot);
                float sp = sin(rot + drot);
                locx = (50 - randx) * c + (50 - randy) * s;
                locy = -(50 - randx) * s + (50 - randy) * c;

                int actx = (locx * cp - locy * sp) + (randx + randdx);
                int acty = (locx * sp + locy * cp) + (randy + randdy);
                movx = actx - 50;
                movy = acty - 50;

                renderImage(WIDTH, HEIGHT, randx, randy, rot, imgPtr, character);
                renderImage(WIDTH, HEIGHT, randx + randdx, randy + randdy, rot + drot, imgPtr + WIDTH * HEIGHT, character);

                // renderRect(WIDTH, HEIGHT, rectW, rectH, randx, randy, rot, imgPtr);
                // renderRect(WIDTH, HEIGHT, rectW, rectH, randx + randdx, randy + randdy, rot + drot, imgPtr + WIDTH * HEIGHT);

                // if ((imgPtr[HEIGHT / 2 * WIDTH + WIDTH / 2] == BK_GROUND) || ((movx > MAX_MOVE) || (movx < -MAX_MOVE) || (movy > MAX_MOVE) || (movy < -MAX_MOVE)))
                //     cout << "." << endl;
            } while ((imgPtr[HEIGHT / 2 * WIDTH + WIDTH / 2] == BK_GROUND) || ((movx > MAX_MOVE) || (movx < -MAX_MOVE) || (movy > MAX_MOVE) || (movy < -MAX_MOVE)));
            // totWasted += (std::chrono::system_clock::now() - startC) * (numTries - 1) / numTries;

            // cout << movx << " " << movy << endl;
            // imgPtr[5050] = -3;
            // imgPtr[5050 + 10000 + movx + movy * 100] = -3;
            // drawDat("test1.png", 100, 100, imgPtr, [](float x) { return x * 64 + 128 + 63; });
            // drawDat("test2.png", 100, 100, imgPtr + 10000, [](float x) { return x * 64 + 128 + 63; });return 0;
            for (int i = 0; i < NUM_LABELS; i++) {
                int px = i % 11 - 5;
                int py = i / 11 - 5;
                labPtr[i] = exp(-((px - movx) * (px - movx) + (py - movy) * (py - movy)) / 3.0);
            }
            // labPtr[0] = movx + MAX_MOVE;
            // labPtr[1] = movy + MAX_MOVE;
        }
        // cout << "Total wasted time: " << totWasted.count() << "s of " << std::chrono::duration<double>(std::chrono::system_clock::now() - start).count() << "s" << endl;
        if (solver->iter() % 100 == 0) {
            float loss;
            net->Forward(&loss);
            cout << "loss at " << solver->iter() << ": " << loss << endl;
        }
        if ((solver->iter() % 1000 == 0) && (solver->iter() > 0)) {
            solver->Snapshot();
        }
        if (solver->iter() > 25000) {
            return 0;
        }

        solver->Step(1);
    }

    return 0;
}
