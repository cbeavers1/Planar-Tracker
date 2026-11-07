#include <opencv2/features2d.hpp>
#include <vector>

class ObjectHighlighter : public VideoProcessor {

public:
    //Constructor and Destructor
    ObjectHighlighter();
    ~ObjectHighlighter();

    // Public functions
    void object_selection();
    void save_video_with_highlights();
    void capture_frame_with_highlights(int);
    void play_video() override;
    void set_color(int, int, int);
    cv::Scalar get_color();
    static void draw_selection(int, int, int, int, void*);

    private:

    // Private functions
    void highlight_objects();

    // Private variables
    std::vector<cv::Mat> templates;
    std::vector<std::vector<cv::Point>> object_points;
    std::vector<cv::Point> xy;
    std::vector<std::vector<cv::KeyPoint>> key_points;
    std::vector<cv::Mat> descriptors;
    std::vector<cv::KeyPoint> k_frame;
    std::vector<std::vector<std::vector<cv::DMatch>>> fl_matches;
    std::vector<std::vector<cv::DMatch>> good_matches;
    std::vector<std::vector<cv::Point2f>> src_points;
    std::vector<std::vector<cv::Point2f>> dst_points;
    std::vector<cv::Mat> masks;
    std::vector<cv::Mat> homographies;
    std::vector<cv::Point2f> homo_dst;
    std::vector<std::vector<cv::Point2f>> corner_pts;
    std::vector<std::vector<cv::Point2f>> dst;
    std::vector<std::vector<cv::Point>> dst_int;
    std::vector<int> h;
    std::vector<int> w;
    cv::Mat curr_frame;
    cv::Mat des_frame;
    cv::Mat selector_frame;
    cv::Mat template_frame;
    cv::Mat overlay_frame;
    cv::Scalar color;
    int idx;
    const int MIN_MATCH_COUNT = 10;
};

// Function definitions

ObjectHighlighter::ObjectHighlighter() {
    std::cout << "Highlighter constructed" << std::endl;
}

ObjectHighlighter::~ObjectHighlighter() {
    std::cout << "Highlighter deconstructed" << std::endl;
}

// Allows user to set the bgr values of the opaque overlay
void ObjectHighlighter::set_color(int b, int g, int r) {
    color = cv::Scalar(b, g, r);
}

// Returns the chosen color
cv::Scalar ObjectHighlighter::get_color() {
    return color;
}

// Handles template creation and calls drawing function
void ObjectHighlighter::object_selection() {
    // Loops through the first 10 frames and uses the last
    while (capture.get(cv::CAP_PROP_POS_FRAMES) < 10) {
        // Loads the last frame
        bool ret = capture.read(selector_frame);

        if (!ret) {
            std::cerr << "Couldn't read frame (object_selection error)" << std::endl;
            break;
        }
    }

    // Clone the chosen frame for template creation
    template_frame = selector_frame.clone();

    while (true) {
        // Set the window size to appropriate size and show it
        cv::namedWindow("Object Selection", cv::WINDOW_NORMAL);
        cv::resizeWindow("Object Selection", cv::Size(400, 600));
        cv::imshow("Object Selection", template_frame);

        // Set the mouse call back to draw on image for object selection
        cv::setMouseCallback("Object Selection", draw_selection, this);

        // Options while drawing
        std::cout << "Press 'd' to reset and try again" << std::endl
                  << "Press 's' to save templates" << std::endl
                  << "================================" << std::endl;

        int key = cv::waitKey(0) & 0xFF;

        // Give option to reset on mistakes by clearing 
        // lines and object selections
        if (key == 'd') {
            xy.clear();
            object_points.clear();
            template_frame = selector_frame.clone();
        } 
        else if (key == 's') { // Uses 's' to continue, saving the values
            cv::destroyAllWindows();
            break;
        }
    }

    // Create bounding rectangles given the polygon shapes drawn
    // Pulls templates from selector given rectangle points
    for (std::vector<cv::Point>& points : object_points) {
        cv::Rect rect = cv::boundingRect(points);
        templates.push_back(selector_frame(rect).clone());
    }
}

// Free-hand drawing for object selection
void ObjectHighlighter::draw_selection(int event, int x, int y, int flags, void* user_data) {
    // Pass object for instance access
    ObjectHighlighter *self = static_cast<ObjectHighlighter*>(user_data);

    if (event == cv::EVENT_LBUTTONDOWN) {
        self->xy.push_back(cv::Point(x,y));
    }
    else if (event == cv::EVENT_LBUTTONUP) {
        self->object_points.push_back(self->xy);

        // Connects the final point to the first point for polygon closure
        cv::polylines(self->template_frame, self->xy, true, cv::Scalar(0, 255, 0), 2);
        cv::imshow("Object Selection", self->template_frame);
        self->xy.clear();
    }
    else if (event == cv::EVENT_MOUSEMOVE) {
        if (flags & cv::EVENT_FLAG_LBUTTON) {
            self->xy.push_back(cv::Point(x,y));
            cv::line(self->template_frame, self->xy[self->xy.size()-2], cv::Point(x,y), cv::Scalar(0,255,0), 2);
            cv::imshow("Object Selection", self->template_frame);
        }
    }
}

// Handles template processing, homography, and covering with highlight
void ObjectHighlighter::highlight_objects() {

    // Clear data for fresh iterations
    for (auto& v : good_matches) v.clear();
    for (auto& v : src_points) v.clear();
    for (auto& v : dst_points) v.clear();
    for (auto& v : dst_int) v.clear();
    for (auto& v : fl_matches) v.clear();

    // Resize vectors to prevent invalid indexing
    src_points.resize(templates.size());
    dst_points.resize(templates.size());
    corner_pts.resize(templates.size());
    homographies.resize(templates.size());
    h.resize(templates.size());
    w.resize(templates.size());
    dst_int.resize(templates.size());
    masks.resize(templates.size());
    dst.resize(templates.size());
    fl_matches.resize(templates.size());
    good_matches.resize(templates.size());
    overlay_frame = curr_frame.clone();

    // Use SIFT to gather keypoints and descriptors
    cv::Ptr<cv::SIFT> sifter = cv::SIFT::create();

    // Skip SIFT reprocessing on templates for efficiency 
    if (key_points.empty()) {
        key_points.resize(templates.size());
        descriptors.resize(templates.size());

        for (int i = 0; i < templates.size(); i++) {
            sifter->detectAndCompute(templates[i], cv::noArray(), key_points[i], descriptors[i]);
        }
    }

    sifter->detectAndCompute(curr_frame, cv::noArray(), k_frame, des_frame);

    cv::FlannBasedMatcher flann(
        cv::makePtr<cv::flann::KDTreeIndexParams>(50),
        cv::makePtr<cv::flann::SearchParams>(250)
    );

    // Gather matches using FLANN 
    for (int j = 0; j < templates.size(); ++j) {
        flann.knnMatch(
            descriptors[j],
            des_frame,
            fl_matches[j],
            2
        );
    }

    // Lowe's ratio test. 
    for (int k = 0; k < fl_matches.size(); ++k) {
        for (auto& matches : fl_matches[k]) {
            if (matches.size() >= 2) {
                const cv::DMatch& m = matches[0];
                const cv::DMatch& n = matches[1];

                // Fairly lenient ratio from imperical testing
                if (m.distance < 0.85 * n.distance) { 
                    good_matches[k].push_back(m);
                }
            }
        }
    }

    // Loop for multiple objects selected
    for (int l = 0; l < good_matches.size(); l++) {
        if (good_matches[l].size() > MIN_MATCH_COUNT) {
            for (const cv::DMatch& m : good_matches[l]) {
                // Gather source points and destinatination points for homography calculation
                src_points[l].push_back(key_points[l][m.queryIdx].pt);
                dst_points[l].push_back(k_frame[m.trainIdx].pt);
            }

            // Calculate homographies and store in vector
            homographies[l] = (cv::findHomography(
                src_points[l],
                dst_points[l],
                cv::RANSAC,
                5.0,
                masks[l]
            ));

            // Height and width data for each template created
            h[l] = (templates[l].rows);
            w[l] = (templates[l].cols);

            // Calculate corners of each template
            corner_pts[l] = {
                cv::Point2f(0, 0),
                cv::Point2f(0, h[l] - 1),
                cv::Point2f(w[l] - 1, h[l] - 1),
                cv::Point2f(w[l] - 1, 0)
            };

            // Next iteration if homographies failed
            if (homographies[l].empty()) {
                continue;
            }

            // Transform template into frame space
            cv::perspectiveTransform(corner_pts[l], dst[l], homographies[l]);

            for (const cv::Point2f& p : dst[l]) {
                dst_int[l].push_back(cv::Point(
                    static_cast<int>(p.x),
                    static_cast<int>(p.y)
                ));
            }

            // Fill rectangle locations on frame copy (overlay)
            cv::fillPoly(
                overlay_frame,
                dst_int[l],
                get_color()
            );
        }
    }

    // Blend overlay and current frame to highlight objects
    cv::addWeighted(overlay_frame, 0.4, curr_frame, 0.6, 0, curr_frame);
}

// Writes video file
void ObjectHighlighter::save_video_with_highlights() {
    capture.set(cv::CAP_PROP_POS_FRAMES, 0.0);
    fps = capture.get(cv::CAP_PROP_FPS);

    int save_type;

    // User selects save type
    std::cout << "Choose a file type" << std::endl
              << "1. MP4" << std::endl
              << "2. AVI" << std::endl;

    std::cin >> save_type;

    cv::VideoWriter video_writer;

    if (save_type == 1) {
        video_writer.open(
            "highlighted_video.mp4",
            cv::VideoWriter::fourcc('m', 'p', '4', 'v'),
            fps,
            cv::Size(
                static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
                static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
            )
        );
    }
    else if (save_type == 2) {
        video_writer.open(
            "highlighted_video.avi",
            cv::VideoWriter::fourcc('M', 'J', 'P', 'G'),
            fps,
            cv::Size(
                static_cast<int>(capture.get(cv::CAP_PROP_FRAME_WIDTH)),
                static_cast<int>(capture.get(cv::CAP_PROP_FRAME_HEIGHT))
            )
        );
    }
    else {
        // Error for bad input
        std::cerr << "Invalid choice" << std::endl;
    }

    // Error if video writer can't open
    if (!video_writer.isOpened()) {
        std::cerr << "Couldn't open video writer" << std::endl;
        return;
    }

    std::cout << "Writing video to file name 'highlighted_video'" << std::endl;

    // Loops through frames, writes each to video file
    while (capture.read(curr_frame)) {
        highlight_objects();
        video_writer.write(curr_frame);
    }

    std::cout << "Finished saving video" << std::endl;

    video_writer.release();
}

// Simple image save function
void ObjectHighlighter::capture_frame_with_highlights(int idx) {
    cv::imwrite("highlighted_frame_" + std::to_string(idx) + ".png", curr_frame);
}

// Overriden function for playing the video with highlights
void ObjectHighlighter::play_video() {
    std::cout << "Press 'a' to rewind the video" << std::endl
              << "Press 's' to save the current frame with highlights" << std::endl
              << "Press 'q' to exit the video" << std::endl
              << "===================================================" << std::endl;

    // Loop to read each frame
    while (capture.read(curr_frame)) {
        highlight_objects();

        // Viewable window size
        cv::namedWindow(name, cv::WINDOW_NORMAL);
        cv::resizeWindow(name, cv::Size(400, 620));
        cv::imshow(name, curr_frame);

        int key = cv::waitKey(1);

        // User choices during playback
        if (key == 'q') {
            cv::destroyAllWindows();
            break;
        }
        else if (key == 'a') {
            // Rewinds 5 seconds each time
            rewind_video(rewind_secs);
        }
        else if (key == 's') {
            // Calls simple image with highlights save function
            capture_frame_with_highlights(static_cast<int>(capture.get(cv::CAP_PROP_POS_FRAMES)));
        }
    }
}
