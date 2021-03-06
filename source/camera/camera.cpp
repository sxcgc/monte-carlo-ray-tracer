#include "camera.hpp"

void Camera::samplePixel(size_t x, size_t y)
{
    auto& pixel = image(x, y);

    double pixel_size = sensor_width / image.width;
    double sub_step = 1.0 / sqrtspp;

    for (int s_x = 0; s_x < sqrtspp; s_x++)
    {
        for (int s_y = 0; s_y < sqrtspp; s_y++)
        {
            glm::dvec2 pixel_space_pos(x + s_x * sub_step + Random::range(0.0, sub_step), y + s_y * sub_step + Random::range(0.0, sub_step));
            glm::dvec2 center_offset = pixel_size * (glm::dvec2(image.width, image.height) / 2.0 - pixel_space_pos);

            glm::dvec3 sensor_pos = eye + forward * focal_length + left * center_offset.x + up * center_offset.y;

            if (photon_map)
            {
                pixel += photon_map->sampleRay(Ray(eye, sensor_pos, scene->ior));
            } 
            else
            {
                pixel += scene->sampleRay(Ray(eye, sensor_pos, scene->ior));
            } 
        }
    }
    pixel /= pow2(static_cast<double>(sqrtspp));
    num_sampled_pixels++;
}

void Camera::sampleImage(std::shared_ptr<Scene> s, std::shared_ptr<PhotonMap> pm)
{
    scene = s;
    photon_map = pm;

    std::vector<Bucket> buckets_vec;
    for (size_t x = 0; x < image.width; x += bucket_size)
    {
        size_t x_end = x + bucket_size;
        if (x_end >= image.width) x_end = image.width;
        for (size_t y = 0; y < image.height; y += bucket_size)
        {
            size_t y_end = y + bucket_size;
            if (y_end >= image.height) y_end = image.height;
            buckets_vec.push_back(Bucket(glm::ivec2(x, y), glm::ivec2(x_end, y_end)));
        }
    }

    std::shuffle(buckets_vec.begin(), buckets_vec.end(), Random::getEngine());
    WorkQueue<Bucket> buckets(buckets_vec);
    buckets_vec.clear();

    std::function<void(Camera*, WorkQueue<Bucket>&)> f = &Camera::sampleImageThread;

    std::vector<std::unique_ptr<std::thread>> threads(scene->num_threads);
    for (auto& thread : threads)
    {
        thread = std::make_unique<std::thread>(f, this, std::ref(buckets));
    }

    std::function<void(Camera*, WorkQueue<Bucket>&)> p = &Camera::printInfoThread;
    std::thread print_thread(p, this, std::ref(buckets));

    print_thread.join();

    for (auto& thread : threads)
    {
        thread->join();
    }
}

void Camera::sampleImageThread(WorkQueue<Bucket>& buckets)
{
    Random::seed(std::random_device{}()); // Each thread uses different seed.

    Bucket bucket;
    while (buckets.getWork(bucket))
    {
        for (size_t x = bucket.min.x; x < bucket.max.x; x++)
        {
            for (size_t y = bucket.min.y; y < bucket.max.y; y++)
            {
                samplePixel(x, y);
            }
        }
    }
}

void Camera::printInfoThread(WorkQueue<Bucket>& buckets)
{
    auto printProgressInfo = [](double progress, size_t msec_duration, size_t sps, std::ostream& out)
    {
        auto ETA = std::chrono::system_clock::now() + std::chrono::milliseconds(msec_duration);

        std::stringstream ss;
        ss << "\rTime remaining: " << Format::timeDuration(msec_duration)
           << " || " << Format::progress(progress)
           << " || ETA: " << Format::date(ETA)
           << " || Samples/s: " << Format::largeNumber(sps) + "    ";

        out << ss.str();
    };

    while (!buckets.empty())
    {
        if (num_sampled_pixels != last_num_sampled_pixels)
        {
            size_t delta_pixels = num_sampled_pixels - last_num_sampled_pixels;
            size_t pixels_left = image.num_pixels - num_sampled_pixels;

            auto now = std::chrono::steady_clock::now();
            auto delta_t = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_update);

            times.push_back(static_cast<double>(delta_pixels) / delta_t.count());
            if (times.size() > num_times)
                times.pop_front();

            // moving average
            double pixels_per_msec = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

            double progress = 100.0 * static_cast<double>(num_sampled_pixels) / image.num_pixels;
            size_t msec_left = static_cast<size_t>(pixels_left / pixels_per_msec);
            size_t sps = static_cast<size_t>(pixels_per_msec * 1000.0 * pow2(static_cast<double>(sqrtspp)));

            printProgressInfo(progress, msec_left, sps, std::cout);

            last_update = now;
            last_num_sampled_pixels = num_sampled_pixels;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}
