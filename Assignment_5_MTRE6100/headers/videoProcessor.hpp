#include <opencv2/opencv.hpp>

class VideoProcessor {
    public:
    // Constructors
    VideoProcessor();
    VideoProcessor(std::string, std::string);

    // Destructor
    ~VideoProcessor();

    // Member variables
    std::string name;

    // Member functions 
    bool load_video();
    bool load_video(std::string, std::string);
    void display_info();
    virtual void play_video();
    void set_rewind_secs(int);
    void rewind_video(int);
    
    // Inheritable variables 
    protected:
    std::string path;
    cv::VideoCapture capture;
    int fps;
    int frame_count;
    int rewind_secs = 5;
    int duration;
};

// Function definitions
VideoProcessor::VideoProcessor() {
    std::cout << "Processor Constructed" << std::endl;
}

// Overloaded constructor for future editing ease
VideoProcessor::VideoProcessor(std::string path, std::string name) {
    this->path = path;
    this->name = name;
    std::cout << "Processor Constructed" << std::endl;
}

// Sanity check on destructing object
VideoProcessor::~VideoProcessor() {
    std::cout << "Destructed Video Processor" << std::endl;
}

// Load video function used if overloaded constructor used
bool VideoProcessor::load_video() {
    bool ret = capture.open(this->path);
    if (!ret) {
        std::cerr << "Video Not Loaded" << std::endl;
        return false;
    }

    std::cout << "Video Loaded" << std::endl;
    return true;
}

// Overloaded function to use if constructor isn't passed data
bool VideoProcessor::load_video(std::string path, std::string name) {
    this->name = name;
    this->path = path;
    bool ret = capture.open(path);
    if (!ret) {
        std::cerr << "Video not loaded" << std::endl;
        return false;
    }

    std::cout << "Video Loaded" << std::endl;
    return true;
}

// Pulls data from capture object and displays using standard cout
void VideoProcessor::display_info() {
    int w = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH));
    int h = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT));
    this->fps = static_cast<int>(capture.get(cv::CAP_PROP_FPS));
    this->frame_count = static_cast<int>(capture.get(cv::CAP_PROP_FRAME_COUNT));

    this->duration = frame_count / fps;
    int duration_minutes = duration / 60;
    int duration_seconds = duration % 60;

    std::cout << "Resolution: " << w << "X" << h << std::endl
            << "Frame Rate: " << fps << " fps" << std::endl
            << "Duration: " << duration_minutes << "m:" << duration_seconds << "s" << std::endl;
}

// Plays video straight through with no highlights 
// Virtual function to be overridden in derived classes
void VideoProcessor::play_video() {
    cv::Mat frame;
    while (capture.read(frame)) {
        cv::imshow(this->name, frame);
        int key = cv::waitKey(1000/this->fps);

        if (key == 'q') {
            break;
        }
        else if (key == 'a') {
            rewind_video(this->rewind_secs);
        }
    }
}

// Rewinds video by replacing current frame with new index
// by subtracting rewind amount
void VideoProcessor::rewind_video(int rewind_seconds = 5) {
    int curr_frame = static_cast<int>(capture.get(cv::CAP_PROP_POS_FRAMES));
    double new_frame = curr_frame - (rewind_seconds * fps);

    if (new_frame <= 0.0) {
        new_frame = 0.0;
    }

    capture.set(cv::CAP_PROP_POS_FRAMES, new_frame);
}

// Used to set the rewind time
// Created for use in future updates
void VideoProcessor::set_rewind_secs(int rewind_secs) {
    this->rewind_secs = rewind_secs;
}


