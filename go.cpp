#include <iostream>
#include <stdlib.h>
#include "stdio.h"
#include <caffe/caffe.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <math.h>
#include <functional>

using namespace std;
using namespace caffe;
using namespace cv;

void renderRect(int imW, int imH, int rW, int rH, int cx, int cy, float rot, float* data) {
    float bkGround = -1;
    float col = 1;
    float c = cos(rot);
    float s = sin(rot);
    for (int y = 0; y < imH; y++) {
        for (int x = 0; x < imW; x++) {
            int locx = (x - cx) * c - (y - cy) * s;
            int locy = (x - cx) * s + (y - cy) * c;
            data[x + imW * y] = (-rW / 2 < locx) && (locx < rW / 2) && (-rH / 2 < locy) && (locy < rH / 2) ? col : bkGround;
        }
    }
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

    srand(time(0));
    ::google::InitGoogleLogging(argv[0]);

    Caffe::set_mode(Caffe::GPU);

    SolverParameter solver_param;
    ReadSolverParamsFromTextFileOrDie("../lenet_solver.prototxt", &solver_param);

    std::shared_ptr<Solver<float> > solver;
    solver.reset(SolverRegistry<float>::CreateSolver(solver_param));

    Net<float>* net = solver->net().get();

    const int BATCH_SIZE = net->input_blobs()[0]->shape(0);
    const int WIDTH = net->input_blobs()[0]->shape(2);
    const int HEIGHT = net->input_blobs()[0]->shape(3);
    const int NUM_LABELS = net->input_blobs()[1]->shape(1);

    const int MAX_MOVE = (int)(sqrt(net->blob_by_name("fc8")->shape(1)) - 1) / 2;

    int rectW = 50;
    int rectH = 50;

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
            do {
                // rot = (rand() % 10000) * 1.0 / 10000.0 * M_PI / 2;
                // randx = rand() % 30 + 35;
                // randy = rand() % 30 + 35;
                // randdx = rand() % 11 - 5;
                // randdy = rand() % 11 - 5;
                // drot = (rand() % 10000) * 1.0 / 10000.0 * M_PI / 5 - M_PI / 10;
                // locx = (50 - randx) * cos(rot) - (50 - randy) * sin(rot);
                // locy = (50 - randx) * sin(rot) + (50 - randy) * cos(rot);
                // movx = locx * cos(rot + drot) + locy * sin(rot + drot) + randx + randdx - 50;
                // movy = -locx * sin(rot + drot) + locy * cos(rot + drot) + randy + randdy - 50;

                rot = (rand() % 10000) * 1.0 / 10000.0 * M_PI / 2;
                drot = (rand() % 10000) * 1.0 / 10000.0 * M_PI / 5 - M_PI / 10;
                randx = rand() % 49 + 26;
                randy = rand() % 49 + 26;
                randdx = rand() % 11 - 5;
                randdy = rand() % 11 - 5;

                float c = cos(rot);
                float s = sin(rot);
                float cp = cos(rot + drot);
                float sp = sin(rot + drot);
                float locx = (50 - randx) * c - (50 - randy) * s;
                float locy = (50 - randx) * s + (50 - randy) * c;

                int actx = (locx * cp + locy * sp) + (randx + randdx);
                int acty = (-locx * sp + locy * cp) + (randy + randdy);
                movx = actx - 50;
                movy = acty - 50;

                renderRect(WIDTH, HEIGHT, rectW, rectH, randx, randy, rot, imgPtr);
                renderRect(WIDTH, HEIGHT, rectW, rectH, randx + randdx, randy + randdy, rot + drot, imgPtr + WIDTH * HEIGHT);

                if ((imgPtr[WIDTH / 2 + WIDTH * (HEIGHT / 2)] != 1) || ((movx > MAX_MOVE) || (movx < -MAX_MOVE) || (movy > MAX_MOVE) || (movy < -MAX_MOVE)))
                    cout << "." << endl;
            } while ((imgPtr[WIDTH / 2 + WIDTH * (HEIGHT / 2)] != 1) || ((movx > MAX_MOVE) || (movx < -MAX_MOVE) || (movy > MAX_MOVE) || (movy < -MAX_MOVE)));
            cout << "+" << endl;
            // cout << movx << " " << movy << endl;
            // drawDat("t1.png", 100, 100, imgPtr, [](float x) { return x == -1 ? 0 : 255; });
            // drawDat("t2.png", 100, 100, imgPtr + 10000, [](float x) { return x == -1 ? 0 : 255; });
            // return 0;

            //Mat rm1(100, 100, CV_32F);
            //Mat rm2(100, 100, CV_32F);
            //for (int x = 0; x < 100; x++) {
            //    for (int y = 0; y < 100; y++) {
            //        rm1.at<float>(x, y) = imgPtr[x + 100 * y] == 1 ? 0 : 255;
            //        rm2.at<float>(x, y) = imgPtr[x + 100 * y + 100 * 100] == 1 ? 0 : 255;
            //    }
            //}
            //imwrite("test1.png", rm1);
            //imwrite("test2.png", rm2);
            //cout << movx << "," << movy << endl;
            //return 0;
            labPtr[0] = movx + 5 + (movy + 5) * 11;
        }
        if (solver->iter() % 100 == 0) {
            float loss;
            net->Forward(&loss);
            cout << "loss at " << solver->iter() << ": " << loss << endl;
        }
        if (solver->iter() % 500 == 0) {
            solver->Snapshot();
        }

        solver->Step(1);
    }

    return 0;
}
