//
// Created by i.getsman on 06.08.2020.
//

#include "projectds_app.hpp"

int main() {
    ProjectDS app({ 1700, 720 }, "Project Decima [HzD Version " DECIMA_VERSION "]");
    app.init();
    app.run();
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR, int) {
    main();
}
