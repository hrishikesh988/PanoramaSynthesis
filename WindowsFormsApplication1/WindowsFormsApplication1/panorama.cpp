
#include "stdafx.h"
#include <iostream>
#include <fstream>
#include <string>
#include "opencv2/opencv_modules.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/stitching/detail/autocalib.hpp"
#include "opencv2/stitching/detail/blenders.hpp"
#include "opencv2/stitching/detail/camera.hpp"
#include "opencv2/stitching/detail/exposure_compensate.hpp"
#include "opencv2/stitching/detail/matchers.hpp"
#include "opencv2/stitching/detail/motion_estimators.hpp"
#include "opencv2/stitching/detail/seam_finders.hpp"
#include "opencv2/stitching/detail/util.hpp"
#include "opencv2/stitching/detail/warpers.hpp"
#include "opencv2/stitching/warpers.hpp"

using namespace std;
using namespace cv;
using namespace cv::detail;

vector<string> img_names;
bool preview = false;
bool try_gpu = false;
double work_megapix = 0.6;
double seam_megapix = 0.1;
double compose_megapix = -1;
float conf_thresh = 1.f;
string features_type = "surf";
string ba_cost_func = "ray";
string ba_refine_mask = "xxxxx";
bool do_wave_correct = true;
WaveCorrectKind wave_correct = detail::WAVE_CORRECT_HORIZ;
bool save_graph = false;
std::string save_graph_to;
string warp_type = "spherical";
int expos_comp_type = ExposureCompensator::GAIN_BLOCKS;
float match_conf = 0.3f;
string seam_find_type = "gc_color";
int blend_type = Blender::MULTI_BAND;
float blend_strength = 5;
string result_name = "C:\\panorama\\output.jpg";

static int parseCmdArgs(int argc, char** argv)
{
	if (argc == 1)
	{
	
		return -1;
	}
	for (int i = 1; i < argc; ++i)
	{
			if (string(argv[i]) == "--preview")
			{
				preview = true;
			}
			else if (string(argv[i]) == "--try_gpu")
			{
				if (string(argv[i + 1]) == "no")
					try_gpu = false;
				else if (string(argv[i + 1]) == "yes")
					try_gpu = true;
				else
				{
					return -1;
				}
				i++;
			}
			else if (string(argv[i]) == "--work_megapix")
			{
				work_megapix = atof(argv[i + 1]);
				i++;
			}
			else if (string(argv[i]) == "--seam_megapix")
			{
				seam_megapix = atof(argv[i + 1]);
				i++;
			}
			else if (string(argv[i]) == "--compose_megapix")
			{
				compose_megapix = atof(argv[i + 1]);
				i++;
			}
			else if (string(argv[i]) == "--result")
			{
				result_name = argv[i + 1];
				i++;
			}
			else if (string(argv[i]) == "--features")
			{
				features_type = argv[i + 1];
				if (features_type == "orb")
					match_conf = 0.3f;
				i++;
			}
			else if (string(argv[i]) == "--match_conf")
			{
				match_conf = static_cast<float>(atof(argv[i + 1]));
				i++;
			}
			else if (string(argv[i]) == "--conf_thresh")
			{
				conf_thresh = static_cast<float>(atof(argv[i + 1]));
				i++;
			}
			else if (string(argv[i]) == "--ba")
			{
				ba_cost_func = argv[i + 1];
				i++;
			}
			else if (string(argv[i]) == "--ba_refine_mask")
			{
				ba_refine_mask = argv[i + 1];
				if (ba_refine_mask.size() != 5)
				{
					return -1;
				}
				i++;
			}
			else if (string(argv[i]) == "--wave_correct")
			{
				if (string(argv[i + 1]) == "no")
					do_wave_correct = false;
				else if (string(argv[i + 1]) == "horiz")
				{
					do_wave_correct = true;
					wave_correct = detail::WAVE_CORRECT_HORIZ;
				}
				else if (string(argv[i + 1]) == "vert")
				{
					do_wave_correct = true;
					wave_correct = detail::WAVE_CORRECT_VERT;
				}
				else
				{
					return -1;
				}
				i++;
			}
			else if (string(argv[i]) == "--save_graph")
			{
				save_graph = true;
				save_graph_to = argv[i + 1];
				i++;
			}
			else if (string(argv[i]) == "--warp")
			{
				warp_type = string(argv[i + 1]);
				i++;
			}
			else if (string(argv[i]) == "--expos_comp")
			{
				if (string(argv[i + 1]) == "no")
					expos_comp_type = ExposureCompensator::NO;
				else if (string(argv[i + 1]) == "gain")
					expos_comp_type = ExposureCompensator::GAIN;
				else if (string(argv[i + 1]) == "gain_blocks")
					expos_comp_type = ExposureCompensator::GAIN_BLOCKS;
				else
				{
					return -1;
				}
				i++;
			}
			else if (string(argv[i]) == "--seam")
			{
				if (string(argv[i + 1]) == "no" ||
					string(argv[i + 1]) == "voronoi" ||
					string(argv[i + 1]) == "gc_color" ||
					string(argv[i + 1]) == "gc_colorgrad" ||
					string(argv[i + 1]) == "dp_color" ||
					string(argv[i + 1]) == "dp_colorgrad")
					seam_find_type = argv[i + 1];
				else
				{
					return -1;
				}
				i++;
			}
			else if (string(argv[i]) == "--blend")
			{
				if (string(argv[i + 1]) == "no")
					blend_type = Blender::NO;
				else if (string(argv[i + 1]) == "feather")
					blend_type = Blender::FEATHER;
				else if (string(argv[i + 1]) == "multiband")
					blend_type = Blender::MULTI_BAND;
				else
				{
					return -1;
				}
				i++;
			}
			else if (string(argv[i]) == "--blend_strength")
			{
				blend_strength = static_cast<float>(atof(argv[i + 1]));
				i++;
			}
			else if (string(argv[i]) == "--output")
			{
				result_name = argv[i + 1];
				i++;
			}
			else
				img_names.push_back(argv[i]);
	}
	if (preview)
	{
		compose_megapix = 0.6;
	}
	return 0;
}


int main(int argc, char* argv[])
{
#if ENABLE_LOG
	int64 app_start_time = getTickCount();
#endif
	cv::setBreakOnError(true);

	int retval = parseCmdArgs(argc, argv);
	if (retval)
		return retval;

	int num_images = static_cast<int>(img_names.size());
	if (num_images < 2)
	{
		return -1;
	}

	double work_scale = 1, seam_scale = 1, compose_scale = 1;
	bool is_work_scale_set = false, is_seam_scale_set = false, is_compose_scale_set = false;

#if ENABLE_LOG
	int64 t = getTickCount();
#endif

	Ptr<FeaturesFinder> finder;
	if (features_type == "surf")
	{
#if defined(HAVE_OPENCV_NONFREE) && defined(HAVE_OPENCV_GPU) && !defined(ANDROID)
		if (try_gpu && gpu::getCudaEnabledDeviceCount() > 0)
			finder = new SurfFeaturesFinderGpu();
		else
#endif
			finder = new SurfFeaturesFinder();
	}
	else if (features_type == "orb")
	{
		finder = new OrbFeaturesFinder();
	}
	else
	{
		return -1;
	}

	Mat full_img, img;
	vector<ImageFeatures> features(num_images);
	vector<Mat> images(num_images);
	vector<Size> full_img_sizes(num_images);
	double seam_work_aspect = 1;

	for (int i = 0; i < num_images; ++i)
	{
		full_img = imread(img_names[i]);
		full_img_sizes[i] = full_img.size();

		if (full_img.empty())
		{
			return -1;
		}
		if (work_megapix < 0)
		{
			img = full_img;
			work_scale = 1;
			is_work_scale_set = true;
		}
		else
		{
			if (!is_work_scale_set)
			{
				work_scale = min(1.0, sqrt(work_megapix * 1e6 / full_img.size().area()));
				is_work_scale_set = true;
			}
			resize(full_img, img, Size(), work_scale, work_scale);
		}
		if (!is_seam_scale_set)
		{
			seam_scale = min(1.0, sqrt(seam_megapix * 1e6 / full_img.size().area()));
			seam_work_aspect = seam_scale / work_scale;
			is_seam_scale_set = true;
		}

		(*finder)(img, features[i]);
		features[i].img_idx = i;

		resize(full_img, img, Size(), seam_scale, seam_scale);
		images[i] = img.clone();
	}

	finder->collectGarbage();
	full_img.release();
	img.release();

	LOG("Pairwise matching");
#if ENABLE_LOG
	t = getTickCount();
#endif
	vector<MatchesInfo> pairwise_matches;
	BestOf2NearestMatcher matcher(try_gpu, match_conf);
	matcher(features, pairwise_matches);
	matcher.collectGarbage();

	if (save_graph)
	{
		ofstream f(save_graph_to.c_str());
		f << matchesGraphAsString(img_names, pairwise_matches, conf_thresh);
	}

	vector<int> indices = leaveBiggestComponent(features, pairwise_matches, conf_thresh);
	vector<Mat> img_subset;
	vector<string> img_names_subset;
	vector<Size> full_img_sizes_subset;
	for (size_t i = 0; i < indices.size(); ++i)
	{
		img_names_subset.push_back(img_names[indices[i]]);
		img_subset.push_back(images[indices[i]]);
		full_img_sizes_subset.push_back(full_img_sizes[indices[i]]);
	}

	images = img_subset;
	img_names = img_names_subset;
	full_img_sizes = full_img_sizes_subset;

	num_images = static_cast<int>(img_names.size());
	if (num_images < 2)
	{
		return -1;
	}

	HomographyBasedEstimator estimator;
	vector<CameraParams> cameras;
	estimator(features, pairwise_matches, cameras);

	for (size_t i = 0; i < cameras.size(); ++i)
	{
		Mat R;
		cameras[i].R.convertTo(R, CV_32F);
		cameras[i].R = R;
	}

	Ptr<detail::BundleAdjusterBase> adjuster;
	if (ba_cost_func == "reproj") adjuster = new detail::BundleAdjusterReproj();
	else if (ba_cost_func == "ray") adjuster = new detail::BundleAdjusterRay();
	else
	{
		return -1;
	}
	adjuster->setConfThresh(conf_thresh);
	Mat_<uchar> refine_mask = Mat::zeros(3, 3, CV_8U);
	if (ba_refine_mask[0] == 'x') refine_mask(0, 0) = 1;
	if (ba_refine_mask[1] == 'x') refine_mask(0, 1) = 1;
	if (ba_refine_mask[2] == 'x') refine_mask(0, 2) = 1;
	if (ba_refine_mask[3] == 'x') refine_mask(1, 1) = 1;
	if (ba_refine_mask[4] == 'x') refine_mask(1, 2) = 1;
	adjuster->setRefinementMask(refine_mask);
	(*adjuster)(features, pairwise_matches, cameras);


	vector<double> focals;
	for (size_t i = 0; i < cameras.size(); ++i)
	{
		focals.push_back(cameras[i].focal);
	}

	sort(focals.begin(), focals.end());
	float warped_image_scale;
	if (focals.size() % 2 == 1)
		warped_image_scale = static_cast<float>(focals[focals.size() / 2]);
	else
		warped_image_scale = static_cast<float>(focals[focals.size() / 2 - 1] + focals[focals.size() / 2]) * 0.5f;

	if (do_wave_correct)
	{
		vector<Mat> rmats;
		for (size_t i = 0; i < cameras.size(); ++i)
			rmats.push_back(cameras[i].R);
		waveCorrect(rmats, wave_correct);
		for (size_t i = 0; i < cameras.size(); ++i)
			cameras[i].R = rmats[i];
	}

#if ENABLE_LOG
	t = getTickCount();
#endif

	vector<Point> corners(num_images);
	vector<Mat> masks_warped(num_images);
	vector<Mat> images_warped(num_images);
	vector<Size> sizes(num_images);
	vector<Mat> masks(num_images);

	for (int i = 0; i < num_images; ++i)
	{
		masks[i].create(images[i].size(), CV_8U);
		masks[i].setTo(Scalar::all(255));
	}

	Ptr<WarperCreator> warper_creator;
#if defined(HAVE_OPENCV_GPU) && !defined(ANDROID)
	if (try_gpu && gpu::getCudaEnabledDeviceCount() > 0)
	{
		if (warp_type == "plane") warper_creator = new cv::PlaneWarperGpu();
		else if (warp_type == "cylindrical") warper_creator = new cv::CylindricalWarperGpu();
		else if (warp_type == "spherical") warper_creator = new cv::SphericalWarperGpu();
	}
	else
#endif
	{
		if (warp_type == "plane") warper_creator = new cv::PlaneWarper();
		else if (warp_type == "cylindrical") warper_creator = new cv::CylindricalWarper();
		else if (warp_type == "spherical") warper_creator = new cv::SphericalWarper();
		else if (warp_type == "fisheye") warper_creator = new cv::FisheyeWarper();
		else if (warp_type == "stereographic") warper_creator = new cv::StereographicWarper();
	}

	if (warper_creator.empty())
	{
		return 1;
	}

	Ptr<RotationWarper> warper = warper_creator->create(static_cast<float>(warped_image_scale * seam_work_aspect));

	for (int i = 0; i < num_images; ++i)
	{
		Mat_<float> K;
		cameras[i].K().convertTo(K, CV_32F);
		float swa = (float)seam_work_aspect;
		K(0, 0) *= swa; K(0, 2) *= swa;
		K(1, 1) *= swa; K(1, 2) *= swa;

		corners[i] = warper->warp(images[i], K, cameras[i].R, INTER_LINEAR, BORDER_REFLECT, images_warped[i]);
		sizes[i] = images_warped[i].size();

		warper->warp(masks[i], K, cameras[i].R, INTER_NEAREST, BORDER_CONSTANT, masks_warped[i]);
	}

	vector<Mat> images_warped_f(num_images);
	for (int i = 0; i < num_images; ++i)
		images_warped[i].convertTo(images_warped_f[i], CV_32F);


	Ptr<ExposureCompensator> compensator = ExposureCompensator::createDefault(expos_comp_type);
	compensator->feed(corners, images_warped, masks_warped);

	Ptr<SeamFinder> seam_finder;
	if (seam_find_type == "no")
		seam_finder = new detail::NoSeamFinder();
	else if (seam_find_type == "voronoi")
		seam_finder = new detail::VoronoiSeamFinder();
	else if (seam_find_type == "gc_color")
	{
#if defined(HAVE_OPENCV_GPU) && !defined(ANDROID)
		if (try_gpu && gpu::getCudaEnabledDeviceCount() > 0)
			seam_finder = new detail::GraphCutSeamFinderGpu(GraphCutSeamFinderBase::COST_COLOR);
		else
#endif
			seam_finder = new detail::GraphCutSeamFinder(GraphCutSeamFinderBase::COST_COLOR);
	}
	else if (seam_find_type == "gc_colorgrad")
	{
#if defined(HAVE_OPENCV_GPU) && !defined(ANDROID)
		if (try_gpu && gpu::getCudaEnabledDeviceCount() > 0)
			seam_finder = new detail::GraphCutSeamFinderGpu(GraphCutSeamFinderBase::COST_COLOR_GRAD);
		else
#endif
			seam_finder = new detail::GraphCutSeamFinder(GraphCutSeamFinderBase::COST_COLOR_GRAD);
	}
	else if (seam_find_type == "dp_color")
		seam_finder = new detail::DpSeamFinder(DpSeamFinder::COLOR);
	else if (seam_find_type == "dp_colorgrad")
		seam_finder = new detail::DpSeamFinder(DpSeamFinder::COLOR_GRAD);
	if (seam_finder.empty())
	{
		return 1;
	}

	seam_finder->find(images_warped_f, corners, masks_warped);

	images.clear();
	images_warped.clear();
	images_warped_f.clear();
	masks.clear();

#if ENABLE_LOG
	t = getTickCount();
#endif

	Mat img_warped, img_warped_s;
	Mat dilated_mask, seam_mask, mask, mask_warped;
	Ptr<Blender> blender;

	double compose_work_aspect = 1;

	for (int img_idx = 0; img_idx < num_images; ++img_idx)
	{

	
		full_img = imread(img_names[img_idx]);
		if (!is_compose_scale_set)
		{
			if (compose_megapix > 0)
				compose_scale = min(1.0, sqrt(compose_megapix * 1e6 / full_img.size().area()));
			is_compose_scale_set = true;

			compose_work_aspect = compose_scale / work_scale;

			warped_image_scale *= static_cast<float>(compose_work_aspect);
			warper = warper_creator->create(warped_image_scale);

			for (int i = 0; i < num_images; ++i)
			{
			
				cameras[i].focal *= compose_work_aspect;
				cameras[i].ppx *= compose_work_aspect;
				cameras[i].ppy *= compose_work_aspect;

		
				Size sz = full_img_sizes[i];
				if (std::abs(compose_scale - 1) > 1e-1)
				{
					sz.width = cvRound(full_img_sizes[i].width * compose_scale);
					sz.height = cvRound(full_img_sizes[i].height * compose_scale);
				}

				Mat K;
				cameras[i].K().convertTo(K, CV_32F);
				Rect roi = warper->warpRoi(sz, K, cameras[i].R);
				corners[i] = roi.tl();
				sizes[i] = roi.size();
			}
		}
		if (abs(compose_scale - 1) > 1e-1)
			resize(full_img, img, Size(), compose_scale, compose_scale);
		else
			img = full_img;
		full_img.release();
		Size img_size = img.size();

		Mat K;
		cameras[img_idx].K().convertTo(K, CV_32F);

		warper->warp(img, K, cameras[img_idx].R, INTER_LINEAR, BORDER_REFLECT, img_warped);

		mask.create(img_size, CV_8U);
		mask.setTo(Scalar::all(255));
		warper->warp(mask, K, cameras[img_idx].R, INTER_NEAREST, BORDER_CONSTANT, mask_warped);

		compensator->apply(img_idx, corners[img_idx], img_warped, mask_warped);

		img_warped.convertTo(img_warped_s, CV_16S);
		img_warped.release();
		img.release();
		mask.release();

		dilate(masks_warped[img_idx], dilated_mask, Mat());
		resize(dilated_mask, seam_mask, mask_warped.size());
		mask_warped = seam_mask & mask_warped;

		if (blender.empty())
		{
			blender = Blender::createDefault(blend_type, try_gpu);
			Size dst_sz = resultRoi(corners, sizes).size();
			float blend_width = sqrt(static_cast<float>(dst_sz.area())) * blend_strength / 100.f;
			if (blend_width < 1.f)
				blender = Blender::createDefault(Blender::NO, try_gpu);
			else if (blend_type == Blender::MULTI_BAND)
			{
				MultiBandBlender* mb = dynamic_cast<MultiBandBlender*>(static_cast<Blender*>(blender));
				mb->setNumBands(static_cast<int>(ceil(log(blend_width) / log(2.)) - 1.));
			}
			else if (blend_type == Blender::FEATHER)
			{
				FeatherBlender* fb = dynamic_cast<FeatherBlender*>(static_cast<Blender*>(blender));
				fb->setSharpness(1.f / blend_width);
			}
			blender->prepare(corners, sizes);
		}

		blender->feed(img_warped_s, mask_warped, corners[img_idx]);
	}

	Mat result, result_mask;
	blender->blend(result, result_mask);
	imwrite(result_name, result);
	return 0;
}