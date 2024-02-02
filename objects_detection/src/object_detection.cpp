#include <fstream>
#include <objects_detection/object_detection.hpp>

ObjectDetection::ObjectDetection() : Node("object_detection") {
    declare_parameters();
    get_parameters();
    create_rclcpp_instances();
    std::ofstream file(filename);
}

void ObjectDetection::declare_parameters() {
    // FIXME: Double parameters are strings to work with rqt dynamic reconfigure.
    declare_parameter("general.filename", "output.yaml");

    declare_parameter("general.pointcloud_topic_name", "/velodyne_points");
    declare_parameter("general.transform.roll", "-0.09");
    declare_parameter("general.transform.pitch", "0.55");
    declare_parameter("general.transform.yaw", "-0.06");
    declare_parameter("general.ground_level_height", "0.07");

    declare_parameter("general.transform.x", "0.0");
    declare_parameter("general.transform.y", "0.0");
    declare_parameter("general.transform.z", "1.35");

    declare_parameter("general.tunnel.height", "1.5");
    declare_parameter("general.tunnel.width", "5.00");
    declare_parameter("general.tunnel.length", "5.00");

    declare_parameter("general.forward.histogram.resolution", "0.1");
    declare_parameter("general.forward.histogram.min", "15");
    declare_parameter("general.forward.histogram.max", "30");
    declare_parameter("general.forward.histogram.column_density_threshold", "30");

    declare_parameter("general.ground.histogram.resolution", "0.1");
    declare_parameter("general.ground.histogram.min", "120");
    declare_parameter("general.ground.histogram.max", "350");
    declare_parameter("general.ground.histogram.a", "3.5");

    declare_parameter("outlier_remover.radius_outlier.neighbors_count", "8");
    declare_parameter("outlier_remover.radius_outlier.radius", "0.1");

    declare_parameter("conveyor_candidates_clusteler.euclidean.tolerance", "0.5");
    declare_parameter("conveyor_candidates_clusteler.euclidean.min_size", "100");
    declare_parameter("conveyor_candidates_clusteler.euclidean.max_size", "3000");

    declare_parameter("support_candidates_clusteler.euclidean.tolerance", "0.3");
    declare_parameter("support_candidates_clusteler.euclidean.min_size", "20");
    declare_parameter("support_candidates_clusteler.euclidean.max_size", "500");
}

void ObjectDetection::get_parameters() {
    filename = get_parameter("general.filename").as_string();
    pointcloud_topic_name = get_parameter("general.pointcloud_topic_name").as_string();
    roll = std::stod(get_parameter("general.transform.roll").as_string());
    pitch = std::stod(get_parameter("general.transform.pitch").as_string());
    yaw = std::stod(get_parameter("general.transform.yaw").as_string());

    x = std::stod(get_parameter("general.transform.x").as_string());
    y = std::stod(get_parameter("general.transform.y").as_string());
    z = std::stod(get_parameter("general.transform.z").as_string());

    tunnel_width = std::stod(get_parameter("general.tunnel.width").as_string());
    tunnel_height = std::stod(get_parameter("general.tunnel.height").as_string());
    tunnel_length = std::stod(get_parameter("general.tunnel.length").as_string());
    ground_level_height = std::stod(get_parameter("general.ground_level_height").as_string());

    forward_resolution = std::stod(get_parameter("general.forward.histogram.resolution").as_string());
    forward_histogram_min = std::stoi(get_parameter("general.forward.histogram.min").as_string());
    forward_histogram_max = std::stoi(get_parameter("general.forward.histogram.max").as_string());
    forward_column_density_threshold =
        std::stoi(get_parameter("general.forward.histogram.column_density_threshold").as_string());

    ground_resolution = std::stod(get_parameter("general.ground.histogram.resolution").as_string());
    ground_histogram_min = std::stoi(get_parameter("general.ground.histogram.min").as_string());
    ground_histogram_max = std::stoi(get_parameter("general.ground.histogram.max").as_string());
    ground_histogram_a = std::stoi(get_parameter("general.ground.histogram.a").as_string());

    conveyor_candidates_clusteler.euclidean_tolerance =
        std::stod(get_parameter("conveyor_candidates_clusteler.euclidean.tolerance").as_string());
    conveyor_candidates_clusteler.euclidean_max_size =
        std::stoi(get_parameter("conveyor_candidates_clusteler.euclidean.max_size").as_string());
    conveyor_candidates_clusteler.euclidean_min_size =
        std::stoi(get_parameter("conveyor_candidates_clusteler.euclidean.min_size").as_string());

    supports_candidates_clusteler.euclidean_tolerance =
        std::stod(get_parameter("support_candidates_clusteler.euclidean.tolerance").as_string());
    supports_candidates_clusteler.euclidean_max_size =
        std::stoi(get_parameter("support_candidates_clusteler.euclidean.max_size").as_string());
    supports_candidates_clusteler.euclidean_min_size =
        std::stoi(get_parameter("support_candidates_clusteler.euclidean.min_size").as_string());
}

void ObjectDetection::create_rclcpp_instances() {
    transformed_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/rotated", 10);
    ground_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/ground_pub_", 10);
    without_ground_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/without_ground_pub_", 10);

    forward_hist_filtered_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/forward_hist_filtered_pub_", 10);
    top_hist_filtered_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/top_hist_filtered_pub_", 10);
    clustered_conveyors_candidates_pub_ =
        create_publisher<sensor_msgs::msg::PointCloud2>("inz/clustered_conveyors_candidates", 10);
    clustered_conveyors_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/clustered_candidates", 10);
    clustered_supports_candidates_pub_ =
        create_publisher<sensor_msgs::msg::PointCloud2>("inz/clustered_supports_candidates", 10);
    merged_density_clouds_pub_ = create_publisher<sensor_msgs::msg::PointCloud2>("inz/merged_density_clouds", 10);
    clustered_supports_candidates_base_link_pub_ =
        create_publisher<sensor_msgs::msg::PointCloud2>("inz/classified_supports", 10);

    conveyors_detection_3d_pub_ = create_publisher<vision_msgs::msg::Detection3DArray>("inz/conveyors_detection", 10);
    supports_detection_3d_pub_ = create_publisher<vision_msgs::msg::Detection3DArray>("inz/supports_detection", 10);

    forward_density_histogram_pub_ = create_publisher<sensor_msgs::msg::Image>("inz/forward_density_histogram", 10);
    forward_density_clustered_histogram_pub_ =
        create_publisher<sensor_msgs::msg::Image>("inz/forward_density_clustered_histogram", 10);

    ground_density_histogram_pub_ = create_publisher<sensor_msgs::msg::Image>("inz/ground_density_histogram", 10);
    ground_density_clustered_histogram_pub_ =
        create_publisher<sensor_msgs::msg::Image>("inz/ground_density_clustered_histogram", 10);

    using std::placeholders::_1;
    const std::string& topic_name = pointcloud_topic_name;
    pointcloud_sub_ = create_subscription<sensor_msgs::msg::PointCloud2>(
        topic_name, 10, std::bind(&ObjectDetection::lidar_callback, this, _1));
}

void ObjectDetection::lidar_callback(const rclcppCloudSharedPtr msg) {
    auto start = std::chrono::high_resolution_clock::now();
    get_parameters();
    RCLCPP_DEBUG(get_logger(), "Pointcloud callback.");
    if (msg->width * msg->height == 0) {
        RCLCPP_WARN(get_logger(), "Empty pointcloud skipping...");
        return;
    }
    auto normalization_start = std::chrono::high_resolution_clock::now();
    auto frame = msg->header.frame_id;
    auto cloud_raw = pcl_utils::convert_point_cloud2_to_cloud_ptr<PointIR>(msg);

    auto cloud = pcl_utils::remove_far_points_from_ros2bag_converter_bug<PointIR>(cloud_raw, 5.0);
    filter_further_than_5m_points_count = cloud->size();
    // Ground Filtering
    Eigen::Vector3d normal_vec;
    double ground_height;

    CloudIRPtr cloud_for_ground_detection(new CloudIR);
    for (const auto& point : cloud->points) {
        if (std::abs(point.y) < tunnel_width / 2.0) {
            cloud_for_ground_detection->push_back(point);
        }
    }
    auto filtered_ground_clouds =
        filter_ground_and_get_normal_and_height(cloud_for_ground_detection, pcl::SACMODEL_PLANE, 800,
                                                ground_level_height, std::ref(normal_vec), std::ref(ground_height));
    auto ground = filtered_ground_clouds.first;
    auto without_ground = filtered_ground_clouds.second;
    filter_ground_points_count = without_ground->size();

    Eigen::Vector3d rpy;
    auto aligned_cloud = align_to_normal(without_ground, normal_vec, ground_height, std::ref(rpy));

    auto normalization_end = std::chrono::high_resolution_clock::now();

    normalization_duration_count =
        std::chrono::duration_cast<std::chrono::microseconds>(normalization_end - normalization_start).count();

    // ============================================
    auto conveyor_clusterization_start = std::chrono::high_resolution_clock::now();

    // Conveyors detection based on position.z and height
    auto clustered_conveyors_candidates = conveyor_candidates_clusteler.euclidean(aligned_cloud);

    auto conveyor_clusterization_end = std::chrono::high_resolution_clock::now();
    conveyor_clusterization_duration_count = std::chrono::duration_cast<std::chrono::microseconds>(
                                                 conveyor_clusterization_end - conveyor_clusterization_start)
                                                 .count();

    if (not clustered_conveyors_candidates.size()) {
        RCLCPP_WARN(get_logger(), "Cannot find any conveyor candidate! Skipping pointcloud");
        clear_markers(frame);
        return;
    }

    // ============================================
    auto conveyors_classification_start = std::chrono::high_resolution_clock::now();

    auto conveyors_candidates_detection_3d_msg = detect_conveyors(clustered_conveyors_candidates, frame);

    Detection3DArrayPtr conveyors_detection_3d_msg;
    conveyors_detection_3d_msg =
        std::make_shared<vision_msgs::msg::Detection3DArray>(*conveyors_candidates_detection_3d_msg);
    CloudIRLPtrs clustered_conveyors(clustered_conveyors_candidates);
    auto it1 = conveyors_detection_3d_msg->detections.begin();
    auto it2 = clustered_conveyors.begin();

    while (it1 != conveyors_detection_3d_msg->detections.end() && it2 != clustered_conveyors.end()) {
        if (it1->results[0].hypothesis.score < 0.5) {
            it1 = conveyors_detection_3d_msg->detections.erase(it1);
            it2 = clustered_conveyors.erase(it2);
        } else {
            ++it1;
            ++it2;
        }
    }

    auto conveyors_classification_end = std::chrono::high_resolution_clock::now();
    std::chrono::duration_cast<std::chrono::microseconds>(conveyors_classification_end - conveyors_classification_start)
        .count();

    if (not clustered_conveyors.size()) {
        RCLCPP_WARN(get_logger(), "Cannot find any conveyor! Skipping pointcloud");
        clear_markers(frame);
        return;
    }

    // ============================================

    auto density_segmentation_start = std::chrono::high_resolution_clock::now();

    auto merged_conveyors_candidates = pcl_utils::merge_clouds<PointIRL>(clustered_conveyors_candidates);
    auto merged_conveyors = pcl_utils::merge_clouds<PointIRL>(clustered_conveyors);
    auto histogram = create_histogram(merged_conveyors, forward_resolution);

    if (not histogram.data.size() or not histogram.data[0].size()) {
        RCLCPP_WARN(get_logger(), "Cannot create a front histogram.");
        auto density_segmentation_end = std::chrono::high_resolution_clock::now();
        density_segmentation_duration_count =
            std::chrono::duration_cast<std::chrono::microseconds>(density_segmentation_end - density_segmentation_start)
                .count();
        clear_markers(frame);
        return;
    }

    auto clustered_histogram = threshold_histogram(histogram, forward_histogram_min, forward_histogram_max);
    auto low_density_cloud = filter_with_density_on_x_image(merged_conveyors, clustered_histogram);

    auto rotated_for_ground_histogram = pcl_utils::rotate<PointIRL>(merged_conveyors, 0.0, -M_PI / 2.0, 0.0);
    auto ground_histogram = create_histogram(rotated_for_ground_histogram, ground_resolution);

    if (not ground_histogram.data.size() or not ground_histogram.data[0].size()) {
        RCLCPP_WARN(get_logger(), "Cannot create a top histogram.");
        auto density_segmentation_end = std::chrono::high_resolution_clock::now();
        density_segmentation_duration_count =
            std::chrono::duration_cast<std::chrono::microseconds>(density_segmentation_end - density_segmentation_start)
                .count();
        clear_markers(frame);
        return;
    }

    auto clustered_ground_histogram = segment_local_peeks(ground_histogram, 10, 3);
    auto high_density_top_cloud = filter_with_density_on_z_image(merged_conveyors, clustered_ground_histogram);

    auto merged_density_cloud{pcl_utils::merge_clouds_and_remove_simillar_points<PointIRL>(
        {low_density_cloud, high_density_top_cloud}, 0.0001)};

    auto density_segmentation_end = std::chrono::high_resolution_clock::now();
    density_segmentation_duration_count =
        std::chrono::duration_cast<std::chrono::microseconds>(density_segmentation_end - density_segmentation_start)
            .count();

    // ============================================
    auto supports_clusterization_start = std::chrono::high_resolution_clock::now();

    // TODO: create function remove label
    auto pc2 = std::make_shared<sensor_msgs::msg::PointCloud2>(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(merged_density_cloud, frame, this));
    auto top_cloud = pcl_utils::convert_point_cloud2_to_cloud_ptr<PointIR>(pc2);

    auto clustered_supports_candidates = supports_candidates_clusteler.euclidean(top_cloud);

    auto supports_clusterization_end = std::chrono::high_resolution_clock::now();
    supports_clusterization_duration_count = std::chrono::duration_cast<std::chrono::microseconds>(
                                                 supports_clusterization_end - supports_clusterization_start)
                                                 .count();

    if (not clustered_supports_candidates.size()) {
        RCLCPP_WARN(get_logger(), "Cannot find any support candidate! Skipping pointcloud");
        clear_markers(frame);
        return;
    }

    // ============================================
    auto supports_classification_start = std::chrono::high_resolution_clock::now();

    auto merged_supports_candidates = pcl_utils::merge_clouds<PointIRL>(clustered_supports_candidates);
    auto supports_candidates_detection_3d_msg = detect_supports(clustered_supports_candidates, frame);

    auto supports_classification_end = std::chrono::high_resolution_clock::now();
    supports_classification_duration_count = std::chrono::duration_cast<std::chrono::microseconds>(
                                                 supports_classification_end - supports_classification_start)
                                                 .count();

    // ============================================
    auto estimation_start = std::chrono::high_resolution_clock::now();

    auto base_link_cloud = pcl_utils::translate<pcl::PointXYZIRL>(merged_supports_candidates, 0, 0, -ground_height);
    auto translated = pcl_utils::rotate<pcl::PointXYZIRL>(base_link_cloud, -rpy[0], -rpy[1], -rpy[2] - 0.06);

    // Move to baselink

    auto estimation_end = std::chrono::high_resolution_clock::now();
    estimation_duration_count =
        std::chrono::duration_cast<std::chrono::microseconds>(estimation_end - estimation_start).count();

    // ============================================

    // Publish
    ground_pub_->publish(pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIR>(ground, frame, this));
    without_ground_pub_->publish(pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIR>(without_ground, frame, this));
    transformed_pub_->publish(pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIR>(aligned_cloud, frame, this));

    clustered_conveyors_candidates_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(merged_conveyors_candidates, frame, this));
    clustered_conveyors_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(merged_conveyors, frame, this));
    conveyors_detection_3d_pub_->publish(*conveyors_candidates_detection_3d_msg);

    forward_density_histogram_pub_->publish(create_image_from_histogram(histogram));
    forward_density_clustered_histogram_pub_->publish(create_image_from_histogram(clustered_histogram));
    forward_hist_filtered_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(low_density_cloud, frame, this));

    ground_density_clustered_histogram_pub_->publish(create_image_from_histogram(clustered_ground_histogram));
    ground_density_histogram_pub_->publish(create_image_from_histogram(ground_histogram));
    top_hist_filtered_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(high_density_top_cloud, frame, this));
    merged_density_clouds_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(merged_density_cloud, frame, this));

    clustered_supports_candidates_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(merged_supports_candidates, frame, this));
    supports_detection_3d_pub_->publish(*supports_candidates_detection_3d_msg);

    clustered_supports_candidates_base_link_pub_->publish(
        pcl_utils::convert_cloud_ptr_to_point_cloud2<PointIRL>(translated, frame, this));
}

void ObjectDetection::clear_markers(const std::string& frame_name) {
    vision_msgs::msg::Detection3DArray empty;
    empty.header.frame_id = frame_name;
    empty.header.stamp = get_clock()->now();
    conveyors_detection_3d_pub_->publish(empty);
    supports_detection_3d_pub_->publish(empty);
}

Histogram ObjectDetection::create_histogram(CloudIRLPtr cloud, double resolution) {
    Histogram histogram_image;
    auto compare_y = [](const PointIRL lhs, const PointIRL rhs) { return lhs.y < rhs.y; };
    auto compare_z = [](const PointIRL lhs, const PointIRL rhs) { return lhs.z < rhs.z; };

    auto point_with_max_y = *std::max_element(cloud->points.begin(), cloud->points.end(), compare_y);
    auto point_with_max_z = *std::max_element(cloud->points.begin(), cloud->points.end(), compare_z);

    if (2 * point_with_max_y.y < 0 or point_with_max_z.z < 0) {
        RCLCPP_WARN_STREAM(get_logger(), "Skipping creating histogram under the surface OXY. Width: "
                                             << 2 * point_with_max_y.y << " height: " << point_with_max_z.z);
        return histogram_image;
    }

    histogram_image.resolution = resolution;
    histogram_image.width = std::abs(2 * point_with_max_y.y);
    histogram_image.height = std::abs(point_with_max_z.z);

    histogram_image.image_width = static_cast<std::size_t>(histogram_image.width / histogram_image.resolution);
    histogram_image.image_height = static_cast<std::size_t>(histogram_image.height / histogram_image.resolution);

    if (histogram_image.image_width == 0 or histogram_image.image_height == 0) {
        return histogram_image;
    }

    histogram_image.data.resize(histogram_image.image_height);

    for (auto& column : histogram_image.data) {
        column.resize(histogram_image.image_width);
    }

    for (const auto& point : cloud->points) {
        const auto image_width_pos = static_cast<std::size_t>((histogram_image.width / 2 - point.y) / resolution);
        const auto image_height_pos = static_cast<std::size_t>(point.z / histogram_image.resolution);

        if (image_width_pos >= histogram_image.image_width or image_height_pos >= histogram_image.image_height) {
            continue;
        }

        ++histogram_image.data[image_height_pos][image_width_pos];
    }
    return histogram_image;
}

sensor_msgs::msg::Image ObjectDetection::create_image_from_histogram(const Histogram& histogram) {
    sensor_msgs::msg::Image image_msg;
    image_msg.height = histogram.image_height;
    image_msg.width = histogram.image_width;
    image_msg.encoding = "mono8";

    if (image_msg.height == 0 or image_msg.width == 0) {
        return image_msg;
    }

    image_msg.step = image_msg.width;
    image_msg.data.resize(image_msg.height * image_msg.step, 0);
    // TODO: another function
    auto max_element = std::numeric_limits<std::size_t>::min();
    for (const auto& col : histogram.data) {
        const auto col_max = *std::max_element(col.begin(), col.end());
        max_element = std::max(col_max, max_element);
    }
    if (not max_element) {
        return image_msg;
    }

    for (std::size_t i = 0; i < image_msg.height; ++i) {
        for (std::size_t j = 0; j < histogram.data[i].size(); ++j) {
            uint8_t intensity = static_cast<uint8_t>(255 * histogram.data[i][j] / max_element);
            image_msg.data[(image_msg.height - i - 1) * image_msg.step + j] = intensity;
        }
    }

    return image_msg;
}

CloudIRLPtr ObjectDetection::filter_with_density_on_x_image(CloudIRLPtr cloud, const Histogram& histogram) {
    const auto width = histogram.width;
    const auto height = histogram.height;
    const auto image_width = histogram.image_width;
    const auto image_height = histogram.image_height;
    const auto resolution = histogram.resolution;

    auto low_density_cloud = CloudIRLPtr(new CloudIRL);
    for (const auto& point : cloud->points) {
        const auto image_width_pos = static_cast<std::size_t>((width / 2 - point.y) / resolution);
        const auto image_height_pos = static_cast<std::size_t>(point.z / resolution);

        if (image_width_pos >= image_width or image_height_pos >= image_height) {
            continue;
        }
        if (histogram.data[image_height_pos][image_width_pos]) {
            low_density_cloud->points.push_back(point);
        }
    }
    low_density_cloud->width = low_density_cloud->size();
    low_density_cloud->height = 1;
    return low_density_cloud;
}

CloudIRLPtr ObjectDetection::filter_with_density_on_z_image(CloudIRLPtr cloud, const Histogram& histogram) {
    const auto width = histogram.width;
    const auto height = histogram.height;
    const auto image_width = histogram.image_width;
    const auto image_height = histogram.image_height;
    const auto resolution = histogram.resolution;

    auto low_density_cloud = CloudIRLPtr(new CloudIRL);
    for (const auto& point : cloud->points) {
        const auto image_width_pos = static_cast<std::size_t>((width / 2 - point.y) / resolution);
        const auto image_length_pos = static_cast<std::size_t>(point.x / resolution);

        if (image_width_pos >= image_width or image_length_pos >= image_height) {
            continue;
        }
        if (histogram.data[image_length_pos][image_width_pos]) {
            low_density_cloud->points.push_back(point);
        }
    }
    low_density_cloud->width = low_density_cloud->size();
    low_density_cloud->height = 1;
    return low_density_cloud;
}

Histogram ObjectDetection::threshold_histogram(const Histogram& histogram, std::size_t min, std::size_t max) {
    auto clustered_histogram(histogram);
    for (auto& col : clustered_histogram.data) {
        for (auto& density : col) {
            auto clamped_density = std::clamp(density, min, max);
            if (density != clamped_density and density != 0) {
                density = 0;
            }
        }
    }
    return clustered_histogram;
}

Histogram ObjectDetection::remove_low_density_columns(const Histogram& histogram, std::size_t threshold) {
    auto clustered_histogram(histogram);

    for (std::size_t i = 0; i < clustered_histogram.data[0].size(); ++i) {
        std::size_t sum = 0;
        for (std::size_t j = 0; j < clustered_histogram.data.size(); ++j) {
            sum += clustered_histogram.data[j][i];
        }
        if (sum < threshold) {
            for (std::size_t j = 0; j < clustered_histogram.data.size(); ++j) {
                clustered_histogram.data[j][i] = 0;
            }
        }
    }
    return clustered_histogram;
}

BoundingBoxArrayPtr ObjectDetection::make_bounding_boxes_from_pointclouds(
    const CloudIRLPtrs& clustered_supports_candidates, const std::string& frame_name) {
    BoundingBoxArrayPtr bounding_boxes(new vision_msgs::msg::BoundingBox3DArray);
    for (const auto& leg : clustered_supports_candidates) {
        vision_msgs::msg::BoundingBox3D bounding_box;
        const float& max_value = std::numeric_limits<float>::max();
        const float& min_value = -std::numeric_limits<float>::max();
        Point max_coords{min_value, min_value, min_value};
        Point min_coords{max_value, max_value, max_value};
        for (const auto& point : leg->points) {
            max_coords.x = std::max(point.x, max_coords.x);
            max_coords.y = std::max(point.y, max_coords.y);
            max_coords.z = std::max(point.z, max_coords.z);

            min_coords.x = std::min(point.x, min_coords.x);
            min_coords.y = std::min(point.y, min_coords.y);
            min_coords.z = std::min(point.z, min_coords.z);
        }

        bounding_box.size.x = std::abs(max_coords.x - min_coords.x);
        bounding_box.size.y = std::abs(max_coords.y - min_coords.y);
        bounding_box.size.z = std::abs(max_coords.z - min_coords.z);

        bounding_box.center.position.x = min_coords.x + bounding_box.size.x / 2.0;
        bounding_box.center.position.y = min_coords.y + bounding_box.size.y / 2.0;
        bounding_box.center.position.z = min_coords.z + bounding_box.size.z / 2.0;

        bounding_boxes->boxes.push_back(bounding_box);
    }
    bounding_boxes->header.frame_id = frame_name;
    bounding_boxes->header.stamp = get_clock()->now();
    return bounding_boxes;
}

vision_msgs::msg::ObjectHypothesisWithPose ObjectDetection::score_conveyor(const vision_msgs::msg::BoundingBox3D bbox) {
    // Left and right side conveyors
    double conveyor_height = 0.6 - ground_level_height;
    double conveyor_position_z = conveyor_height / 2.0 + ground_level_height;

    std::string class_name = "conveyor_0_6m";
    if (bbox.center.position.y < 0) {
        conveyor_height = 0.7 - ground_level_height;
        conveyor_position_z = conveyor_height / 2.0 + ground_level_height;

        class_name = "conveyor_0_7m";
    }

    vision_msgs::msg::ObjectHypothesisWithPose object;
    auto z_error = std::abs(bbox.center.position.z - conveyor_position_z) / conveyor_position_z;
    auto height_error = std::abs(bbox.size.z - conveyor_height) / conveyor_height;
    auto whole_error = z_error + height_error;

    object.hypothesis.score = std::max({0.0, 1.0 - whole_error});
    object.hypothesis.class_id = class_name;
    return object;
}

Detection3DArrayPtr ObjectDetection::detect_conveyors(const CloudIRLPtrs& clustered_supports_candidates,
                                                      const std::string& frame_name) {
    auto bboxes = make_bounding_boxes_from_pointclouds(clustered_supports_candidates, frame_name);
    Detection3DArrayPtr detections(new vision_msgs::msg::Detection3DArray);
    detections->header.frame_id = frame_name;
    detections->header.stamp = get_clock()->now();

    int i = 0;
    for (const auto& bbox : bboxes->boxes) {
        vision_msgs::msg::Detection3D detection;
        detection.header = detections->header;
        detection.bbox = bbox;
        detection.results.push_back(score_conveyor(bbox));
        detections->detections.push_back(detection);
    }

    return detections;
}

Detection3DArrayPtr ObjectDetection::detect_supports(const CloudIRLPtrs& clustered_supports_candidates,
                                                     const std::string& frame_name) {
    auto bboxes = make_bounding_boxes_from_pointclouds(clustered_supports_candidates, frame_name);
    Detection3DArrayPtr detections(new vision_msgs::msg::Detection3DArray);
    detections->header.frame_id = frame_name;
    detections->header.stamp = get_clock()->now();

    for (auto i = 0u; i < bboxes->boxes.size(); ++i) {
        vision_msgs::msg::Detection3D detection;
        auto& bbox = bboxes->boxes[i];
        auto& cloud = clustered_supports_candidates[i];
        detection.header = detections->header;
        detection.bbox = bbox;

        auto compare_z = [](const PointIRL lhs, const PointIRL rhs) { return lhs.z < rhs.z; };

        auto point_with_max_z = *std::max_element(cloud->points.begin(), cloud->points.end(), compare_z);
        auto point_with_min_z = *std::min_element(cloud->points.begin(), cloud->points.end(), compare_z);

        // Left and right side conveyors
        double support_height = 0.6;
        std::string class_name = "support_0_6m";
        if (point_with_max_z.y < 0) {
            support_height = 0.7;
            class_name = "support_0_7m";
        }

        vision_msgs::msg::ObjectHypothesisWithPose object;
        // The bottom of the support is not in range
        if (point_with_min_z.ring == 0) {
            const auto height = point_with_max_z.z - point_with_min_z.z;
            const auto real_height_in_range = support_height - point_with_min_z.z;

            const auto height_error = std::abs(height - real_height_in_range) / real_height_in_range;
            object.hypothesis.score = 1.0 - height_error;

            // The top of the support is not in range
        } else if (point_with_max_z.ring == 15) {
            const auto height = point_with_max_z.z - point_with_min_z.z;
            const auto real_height_in_range = point_with_max_z.z;

            const auto height_error = std::abs(height - real_height_in_range) / real_height_in_range;
            object.hypothesis.score = 1.0 - height_error;

            // Whole support in the range
        } else {
            const auto height = point_with_max_z.z - point_with_min_z.z + ground_level_height;
            const auto height_error = std::abs(height - support_height) / support_height;

            object.hypothesis.score = 1.0 - height_error;
        }

        if (object.hypothesis.score > 1.0 or object.hypothesis.score < 0.0) {
            object.hypothesis.score = 0.0;
        }

        object.hypothesis.class_id = class_name;
        detection.results.push_back(object);

        detections->detections.push_back(detection);
    }

    return detections;
}

std::pair<CloudIRPtr, CloudIRPtr> ObjectDetection::filter_ground_and_get_normal_and_height(
    CloudIRPtr cloud, int sac_model, int iterations, double radius, Eigen::Vector3d& normal, double& ground_height,
    double eps) {
    // Segment ground
    pcl::ModelCoefficients::Ptr coefficients(new pcl::ModelCoefficients);
    pcl::PointIndices::Ptr inliers(new pcl::PointIndices);
    CloudIRPtr ground(new CloudIR);
    CloudIRPtr without_ground(new CloudIR);

    pcl::SACSegmentation<pcl::PointXYZIR> seg;
    seg.setOptimizeCoefficients(true);
    seg.setModelType(sac_model);
    seg.setMethodType(pcl::SAC_RANSAC);
    seg.setDistanceThreshold(radius);
    seg.setMaxIterations(iterations);
    seg.setEpsAngle(eps);
    pcl::ExtractIndices<pcl::PointXYZIR> extract;
    seg.setInputCloud(cloud);
    seg.segment(*inliers, *coefficients);
    if (inliers->indices.size() == 0) {
        std::cerr << "Could not estimate a planar model for the given dataset." << std::endl;
        // TODO: check it above
        return {nullptr, nullptr};
    }
    extract.setNegative(true);
    extract.setInputCloud(cloud);
    extract.setIndices(inliers);
    extract.filter(*without_ground);
    extract.setNegative(false);
    extract.filter(*ground);

    normal << coefficients->values[0], coefficients->values[1], coefficients->values[2];
    ground_height = coefficients->values[3];
    return {ground, without_ground};
}

CloudIRPtr ObjectDetection::align_to_normal(CloudIRPtr cloud, const Eigen::Vector3d& normal, double ground_height,
                                            Eigen::Vector3d& rpy) {
    const auto& up_vector = Eigen::Vector3d::UnitZ();
    Eigen::Vector3d axis = normal.cross(up_vector).normalized();
    float angle = acos(normal.dot(up_vector) / (normal.norm() * up_vector.norm()));

    Eigen::Matrix3d rotation_matrix;
    rotation_matrix = Eigen::AngleAxisd(angle, axis);
    RCLCPP_DEBUG_STREAM(get_logger(), "Rotation matrix: \n" << rotation_matrix);
    rpy = rotation_matrix.normalized().eulerAngles(2, 1, 0);
    auto transformed_cloud = pcl_utils::translate<pcl::PointXYZIR>(
        pcl_utils::rotate<pcl::PointXYZIR>(cloud, rpy[0], rpy[1], rpy[2] + 0.06), 0, 0, ground_height);
    transformed_cloud->height = transformed_cloud->points.size();
    transformed_cloud->width = 1;
    return transformed_cloud;
}

ObjectDetection::EllipsoidInfo ObjectDetection::get_ellipsoid_and_center(CloudIPtr cloud) {
    const float& max_value = std::numeric_limits<float>::max();
    const float& min_value = -std::numeric_limits<float>::max();
    Point max_coords{min_value, min_value, min_value};
    Point min_coords{max_value, max_value, max_value};
    for (const auto& point : cloud->points) {
        max_coords.x = std::max(point.x, max_coords.x);
        max_coords.y = std::max(point.y, max_coords.y);
        max_coords.z = std::max(point.z, max_coords.z);

        min_coords.x = std::min(point.x, min_coords.x);
        min_coords.y = std::min(point.y, min_coords.y);
        min_coords.z = std::min(point.z, min_coords.z);
    }
    Ellipsoid ellipsoid;
    ellipsoid.radius_x = (max_coords.x - min_coords.x) / 2;
    ellipsoid.radius_y = (max_coords.y - min_coords.y) / 2;
    ellipsoid.radius_z = (max_coords.z - min_coords.z) / 2;
    Point center;
    center.x = min_coords.x + ellipsoid.radius_x;
    center.y = min_coords.y + ellipsoid.radius_y;
    center.z = min_coords.z + ellipsoid.radius_z;
    RCLCPP_DEBUG_STREAM(get_logger(), "Ellipsoid radiuses: " << ellipsoid << "\n ellipsoid center: \n" << center);

    return {ellipsoid, center, "unknown"};
}

void ObjectDetection::save_data_to_yaml(const std::list<EllipsoidInfo>& ellipsoids_infos) {
    YAML::Node frame_node;
    YAML::Node yaml_node;
    std::size_t detected_count;
    frame_node["time"] = rclcpp::Clock{}.now().seconds();
    for (const auto& info : ellipsoids_infos) {
        YAML::Node ellipsoidNode;
        ellipsoidNode["position"]["x"] = info.center.x;
        ellipsoidNode["position"]["y"] = info.center.y;
        ellipsoidNode["position"]["z"] = info.center.z;

        ellipsoidNode["major_axes"]["x"] = info.radiuses.radius_x;
        ellipsoidNode["major_axes"]["y"] = info.radiuses.radius_y;
        ellipsoidNode["major_axes"]["z"] = info.radiuses.radius_z;

        ellipsoidNode["class"] = info.class_name;
        if (info.class_name != "unknown") {
            ++detected_count;
        }

        frame_node["detected_ellipsoids"].push_back(ellipsoidNode);
    }
    std::ofstream file(filename, std::ios::app);

    auto start = std::chrono::high_resolution_clock::now();

    frame_node["durations"]["normalization"] = normalization_duration_count / 10e6;
    frame_node["durations"]["conveyor_clusterization"] = conveyor_clusterization_duration_count / 10e6;
    frame_node["durations"]["conveyor_classification"] = conveyor_classification_duration_count / 10e6;
    frame_node["durations"]["density_segmentation"] = density_segmentation_duration_count / 10e6;
    frame_node["durations"]["supports_clusterization"] = supports_clusterization_duration_count / 10e6;
    frame_node["durations"]["supports_classification"] = supports_classification_duration_count / 10e6;
    frame_node["durations"]["estimation"] = estimation_duration_count / 10e6;
    auto processing_duration = normalization_duration_count + density_segmentation_duration_count +
                               conveyor_clusterization_duration_count + conveyor_classification_duration_count +
                               supports_clusterization_duration_count + supports_classification_duration_count +
                               estimation_duration_count;

    frame_node["durations"]["processing"] = processing_duration / 10e6;

    frame_node["filters_point_sizes"]["original"] = original_points_count;
    frame_node["filters_point_sizes"]["5m_filter"] = filter_further_than_5m_points_count;
    frame_node["filters_point_sizes"]["ground_filter"] = filter_ground_points_count;
    frame_node["filters_point_sizes"]["roi"] = roi_points_count;

    yaml_node.push_back(frame_node);
    if (file.is_open()) {
        file << yaml_node << std::endl;
        auto end = std::chrono::high_resolution_clock::now();
        auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        RCLCPP_DEBUG_STREAM(get_logger(), "Data saved to file. Saving took: " << microseconds / 10e3 << "ms");
    } else {
        RCLCPP_ERROR(get_logger(), "Cannot save data!");
    }
}

MarkersPtr ObjectDetection::make_markers_from_ellipsoids_infos(const std::list<EllipsoidInfo>& ellipsoids_infos) {
    auto marker_array_msg = std::make_shared<visualization_msgs::msg::MarkerArray>();
    std::size_t count_ = 0;

    for (const auto& info : ellipsoids_infos) {
        visualization_msgs::msg::Marker marker;
        marker.header.frame_id = "velodyne";
        marker.ns = "spheres";
        marker.id = count_;

        marker.type = visualization_msgs::msg::Marker::SPHERE;
        marker.action = visualization_msgs::msg::Marker::ADD;
        marker.pose.position.x = info.center.x;
        marker.pose.position.y = info.center.y;
        marker.pose.position.z = info.center.z;

        marker.scale.x = info.radiuses.radius_x * 2;
        marker.scale.y = info.radiuses.radius_y * 2;
        marker.scale.z = info.radiuses.radius_z * 2;  // Height of the cylinder
        marker.color.a = 0.2;                         // Alpha
        marker.color.r = 0.0;                         // Red
        marker.color.g = 0.0;                         // Green
        marker.color.b = 0.0;                         // Blue

        if (info.class_name == "0.6m_height_support") {
            marker.color.r = 0.0;
            marker.color.g = 1.0;
            marker.color.b = 0.0;
            RCLCPP_DEBUG_STREAM(get_logger(), "Found: 0.6m_height_support");
        } else if (info.class_name == "0.7m_height_support") {
            marker.color.r = 0.7;
            marker.color.g = 1.0;
            marker.color.b = 0.3;
            RCLCPP_DEBUG_STREAM(get_logger(), "Found: 0.7m_height_support");

        } else if (info.class_name == "unknown") {
            marker.color.r = 1.0;
            marker.color.g = 0.0;
            marker.color.b = 0.0;
        }
        count_++;
        marker_array_msg->markers.push_back(marker);
    }

    for (std::size_t i = ellipsoids_infos.size(); i < max_detected_legs; ++i) {
        visualization_msgs::msg::Marker marker;
        marker.header.frame_id = "velodyne";
        marker.ns = "spheres";
        marker.id = i;
        marker.action = visualization_msgs::msg::Marker::DELETE;
        marker_array_msg->markers.push_back(marker);
    }
    return marker_array_msg;
}

std::ostream& operator<<(std::ostream& os, const ObjectDetection::Ellipsoid& ellipsoid) {
    os << "Ellipsoid(" << ellipsoid.radius_x << ", " << ellipsoid.radius_y << ", " << ellipsoid.radius_z << ")";
    return os;
}

Histogram ObjectDetection::segment_local_peeks(const Histogram& histogram, std::size_t slope, std::size_t range) {
    Histogram segmented_histogram(histogram);
    for (auto i = range; i < histogram.data.size() - range; ++i) {
        for (auto j = 0u; j < histogram.data[i].size(); ++j) {
            for (auto k = 1u; k <= range; ++k) {
                if (segmented_histogram.data[i][j] != 0 && (histogram.data[i][j] < histogram[i - k][j] + slope ||
                                                            histogram.data[i][j] < histogram.data[i + k][j] + slope)) {
                    segmented_histogram.data[i][j] = 0;
                }
            }
        }
    }
    return segmented_histogram;
}
