#include <opencv2/opencv.hpp>
#include "neurax/hal/AcceleratorFactory.hpp"
#include "neurax/tensor/DataType.hpp"
#include "neurax/tensor/Shape.hpp"
#include "neurax/tensor/Tensor.hpp"
#include "neurax/image/ImageProcessor.hpp"

// Helper: Convert cv::Mat (RGBA, 8UC4) to neurax tensor
neurax::tensor::Tensor from_mat(const cv::Mat &mat)
{
    if (mat.empty() || mat.channels() != 4 || mat.type() != CV_8UC4)
    {
        throw std::runtime_error("Input cv::Mat must be RGBA, 8-bit");
    }
    size_t width = mat.cols;
    size_t height = mat.rows;
    neurax::tensor::Shape shape({1, height, width, 4});
    neurax::tensor::Tensor tensor = neurax::tensor::Tensor::zeros(shape, neurax::tensor::DataType::FLOAT32);
    auto *tensor_data = tensor.data_ptr<float>();
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            const cv::Vec4b &px = mat.at<cv::Vec4b>(y, x);
            size_t idx = (y * width + x) * 4;
            tensor_data[idx + 0] = static_cast<float>(px[0]) / 255.0f; // R
            tensor_data[idx + 1] = static_cast<float>(px[1]) / 255.0f; // G
            tensor_data[idx + 2] = static_cast<float>(px[2]) / 255.0f; // B
            tensor_data[idx + 3] = static_cast<float>(px[3]) / 255.0f; // A
        }
    }
    return tensor;
}

// Helper: Convert neurax tensor (NHWC, RGBA, float32) to cv::Mat (RGBA, 8UC4)
cv::Mat to_mat(const neurax::tensor::Tensor &tensor)
{
    const auto &shape = tensor.shape();
    if (shape.size() != 4 || shape[0] != 1 || shape[3] != 4)
    {
        throw std::runtime_error("Tensor must be NHWC, RGBA");
    }
    int height = static_cast<int>(shape[1]);
    int width = static_cast<int>(shape[2]);
    cv::Mat mat(height, width, CV_8UC4);
    const float *tensor_data = tensor.data_ptr<float>();
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            int idx = (y * width + x) * 4;
            mat.at<cv::Vec4b>(y, x)[0] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, tensor_data[idx + 0])) * 255.0f)); // R
            mat.at<cv::Vec4b>(y, x)[1] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, tensor_data[idx + 1])) * 255.0f)); // G
            mat.at<cv::Vec4b>(y, x)[2] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, tensor_data[idx + 2])) * 255.0f)); // B
            mat.at<cv::Vec4b>(y, x)[3] = static_cast<uint8_t>(std::round(std::min(1.0f, std::max(0.0f, tensor_data[idx + 3])) * 255.0f)); // A
        }
    }
    return mat;
}

neurax::tensor::Tensor create_gaussian_blur_kernel()
{
    size_t input_channels = 4;  // RGBA
    size_t output_channels = 4; // RGBA
    unsigned long size = 3;
    neurax::tensor::Tensor kernel({size, size, input_channels, output_channels}, neurax::tensor::DataType::FLOAT32);
    auto data = kernel.data_ptr<float>();

    float sigma = 1.0f;
    int half = size / 2;

    // 2D Gaussian kernel
    std::vector<float> gaussian(size * size);
    float sum = 0.0f;
    for (int x = -half; x <= half; ++x)
    {
        for (int y = -half; y <= half; ++y)
        {
            float value = std::exp(-(x * x + y * y) / (2 * sigma * sigma));
            value /= (2.0f * static_cast<float>(M_PI) * sigma * sigma);
            gaussian[(x + half) * size + (y + half)] = value;
            sum += value;
        }
    }
    for (auto &v : gaussian)
        v /= sum; // normalizacija na 1

    // Popuni kernel: RGB dobija Gaussian, A dobija identity
    for (int out_ch = 0; out_ch < (int)output_channels; out_ch++)
    {
        for (int in_ch = 0; in_ch < (int)input_channels; in_ch++)
        {
            for (int x = 0; x < (int)size; ++x)
            {
                for (int y = 0; y < (int)size; ++y)
                {
                    size_t weight_idx =
                        (((x)*size + (y)) * input_channels + in_ch) * output_channels + out_ch;

                    if (out_ch < 3 && in_ch == out_ch)
                    {
                        // Blur samo ako je isti kanal (R->R, G->G, B->B)
                        data[weight_idx] = gaussian[x * size + y];
                    }
                    else if (out_ch == 3 && in_ch == 3)
                    {
                        // A kanal = identity (centar = 1)
                        data[weight_idx] = (x == half && y == half) ? 1.0f : 0.0f;
                    }
                    else
                    {
                        // nema miješanja kanala
                        data[weight_idx] = 0.0f;
                    }
                }
            }
        }
    }

    return kernel;
}

int main()
{
    // Create a VideoCapture object to access the default webcam (index 0)
    cv::VideoCapture cap(0);

    // Check if the webcam was opened successfully
    if (!cap.isOpened())
    {
        std::cerr << "Error: Could not open webcam." << std::endl;
        return -1;
    }

    // Create a window to display the video feed
    cv::namedWindow("Webcam Feed", cv::WINDOW_NORMAL);

    // Mat object to store video frames
    cv::Mat frame;

    neurax::image::ImageProcessor processor;
    auto accelerator = neurax::hal::AcceleratorFactory::create(neurax::hal::AcceleratorType::CPU_OPTIMIZED);
    accelerator->initialize();
    neurax::tensor::Tensor blur_kernel = create_gaussian_blur_kernel();
    neurax::hal::ConvolutionConfig c(3, 1, 1, 4, 4);

    while (true)
    {
        std::cout << "Starting main loop iteration\n";
        // Read a new frame from the webcam
        cap >> frame;

        // Check if the frame is empty (e.g., if the webcam was disconnected)
        if (frame.empty())
        {
            std::cerr << "Error: Empty frame received, exiting." << std::endl;
            break;
        }

        bool success = cv::imwrite("output.png", frame);
        if (!success)
            std::cerr << "Failed to write image!" << std::endl;

        // Convert frame to RGBA if needed
        cv::Mat rgba_frame;
        cv::cvtColor(frame, rgba_frame, cv::COLOR_BGR2RGBA);

        // Load frame into processor
        auto input_tensor = from_mat(rgba_frame);

        auto out_hal = accelerator->convolution(
            input_tensor, blur_kernel,
            neurax::tensor::Tensor::zeros({1, 3, 3, 1}, neurax::tensor::DataType::FLOAT32), c);

        // Convert output tensor back to cv::Mat
        cv::Mat blurred_frame = to_mat(out_hal);

        // Display the frame in the window
        cv::imshow("Blurred Webcam", blurred_frame);

        // Just for testing
        // if (cv::imwrite("webcam_blur.png", blurred_frame))
        //     std::cerr << "\nImage saved successfully!\n";

        // break;

        // Wait for 25 milliseconds and check for a key press
        // If 'Esc' (ASCII 27) is pressed, exit the loop
        if (cv::waitKey(1) == 'q')
        {
            break;
        }
    }

    accelerator->cleanup();
    // Release the VideoCapture object and close all OpenCV windows
    cap.release();
    cv::destroyAllWindows();

    return 0;
}