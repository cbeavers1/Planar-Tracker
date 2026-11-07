#include <iostream>
#include <videoProcessor.hpp>
#include <objectHighlighter.hpp>

int main(int argc, char* argv[]) {

    ObjectHighlighter highlighter;
    std::string path;
    std::string name;
    int b;
    int g;
    int r;
    int choice;
    bool ret = false;

    // Loop to enter correct video file path
    while (!ret) {
        std::cout << "Enter video file path" << std::endl;
        std::cin >> path;
        std::cout << "Enter a name for your video" << std::endl;
        std::cin >> name;
    
        ret = highlighter.load_video(path, name);
    }

    // Automatically starts object selection on given video
    std::cout << "Select the objects you would like to track" << std::endl;
    highlighter.object_selection();

    // User selects color
    std::cout << "Choose a color for highlighting (BGR)" << std::endl;
    std::cout << "B: ";
    std::cin >> b;
    std::cout << "G: ";
    std::cin >> g;
    std::cout << "R: ";
    std::cin >> r;

    highlighter.set_color(b, g, r);

    // Loop for options to play or save video using switch 
    while (choice != 0) {
        std::cout << "Choose what to do next" << std::endl
                << "1. Play video with highlights" << std::endl
                << "2. Save video with highlights" << std::endl
                << "3. Exit the program" << std::endl;

        std::cin >> choice;

        switch (choice){
            case 1:
            highlighter.play_video();
            break;

            case 2:
            highlighter.save_video_with_highlights();
            break;

            case 3:
            std::cout << "Exiting program" << std::endl;
            exit(0);

            default:
            std::cout << "Invalid choice" << std::endl;
        }
    }
    
    return 0;
}