#include "avisynth_c.h"
#include "lib/extras/codec.h"
#include "lib/jxl/color_management.h"
#include "lib/jxl/enc_color_management.h"
#include "tools/ssimulacra.h"
#include "tools/ssimulacra2.h"

struct ssimulacra_data
{
    AVS_Clip* clip1;
    jxl::CodecInOut ref;
    jxl::CodecInOut dist;
    int simple;

    void(*fill)(jxl::CodecInOut& __restrict ref, jxl::CodecInOut& __restrict dist, AVS_VideoFrame* src1, AVS_VideoFrame* src2) noexcept;
};

template <typename T>
static void fill_image_ssimulacra(jxl::CodecInOut& __restrict ref, jxl::CodecInOut& __restrict dist, AVS_VideoFrame* src1, AVS_VideoFrame* src2) noexcept
{
    const size_t stride1{ avs_get_pitch(src1) / sizeof(T) };
    const size_t stride2{ avs_get_pitch(src2) / sizeof(T) };
    const int height{ avs_get_height(src1) };
    const size_t width{ avs_get_row_size(src1) / sizeof(T) };

    const int planes[3] = { AVS_PLANAR_R, AVS_PLANAR_G, AVS_PLANAR_B };

    if constexpr (std::is_same_v<T, uint8_t>)
    {
        jxl::Image3B tmp1(width, height);
        jxl::Image3B tmp2(width, height);

        for (int i{ 0 }; i < 3; ++i)
        {
            const uint8_t* srcp1{ avs_get_read_ptr_p(src1, planes[i]) };
            const uint8_t* srcp2{ avs_get_read_ptr_p(src2, planes[i]) };

            for (int y{ 0 }; y < height; ++y)
            {
                memcpy(tmp1.PlaneRow(i, y), srcp1, width * sizeof(T));
                memcpy(tmp2.PlaneRow(i, y), srcp2, width * sizeof(T));

                srcp1 += stride1;
                srcp2 += stride2;
            }
        }

        ref.SetFromImage(jxl::ConvertToFloat(tmp1), jxl::ColorEncoding::SRGB(false));
        dist.SetFromImage(jxl::ConvertToFloat(tmp2), jxl::ColorEncoding::SRGB(false));
    }
    else if constexpr (std::is_same_v<T, uint16_t>)
    {
        jxl::Image3U tmp1(width, height);
        jxl::Image3U tmp2(width, height);

        for (int i{ 0 }; i < 3; ++i)
        {
            const uint16_t* srcp1{ reinterpret_cast<const uint16_t*>(avs_get_read_ptr_p(src1, planes[i])) };
            const uint16_t* srcp2{ reinterpret_cast<const uint16_t*>(avs_get_read_ptr_p(src2, planes[i])) };

            for (int y{ 0 }; y < height; ++y)
            {
                memcpy(tmp1.PlaneRow(i, y), srcp1, width * sizeof(T));
                memcpy(tmp2.PlaneRow(i, y), srcp2, width * sizeof(T));

                srcp1 += stride1;
                srcp2 += stride2;
            }
        }

        ref.SetFromImage(jxl::ConvertToFloat(tmp1), jxl::ColorEncoding::SRGB(false));
        dist.SetFromImage(jxl::ConvertToFloat(tmp2), jxl::ColorEncoding::SRGB(false));
    }
    else
    {
        jxl::Image3F tmp1(width, height);
        jxl::Image3F tmp2(width, height);

        for (int i{ 0 }; i < 3; ++i)
        {
            const float* srcp1{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src1, planes[i])) };
            const float* srcp2{ reinterpret_cast<const float*>(avs_get_read_ptr_p(src2, planes[i])) };

            for (int y{ 0 }; y < height; ++y)
            {
                memcpy(tmp1.PlaneRow(i, y), srcp1, width * sizeof(T));
                memcpy(tmp2.PlaneRow(i, y), srcp2, width * sizeof(T));

                srcp1 += stride1;
                srcp2 += stride2;
            }
        }

        ref.SetFromImage(std::move(tmp1), jxl::ColorEncoding::SRGB(false));
        dist.SetFromImage(std::move(tmp2), jxl::ColorEncoding::SRGB(false));
    }
}

template <int feature>
static AVS_VideoFrame* AVSC_CC get_frame_ssimulacra(AVS_FilterInfo* fi, int n)
{
    ssimulacra_data* d{ static_cast<ssimulacra_data*>(fi->user_data) };

    AVS_VideoFrame* src1{ avs_get_frame(fi->child, n) };
    if (!src1)
        return nullptr;

    AVS_VideoFrame* src2{ avs_get_frame(d->clip1, n) };

    d->fill(d->ref, d->dist, src1, src2);

    avs_make_writable(fi->env, &src1);
    AVS_Map* props{ avs_get_frame_props_rw(fi->env, src1) };

    if constexpr (feature)
    {
        Msssim msssim{ ComputeSSIMULACRA2(d->ref.Main(), d->dist.Main()) };
        avs_prop_set_float(fi->env, props, "SSIMULACRA2", msssim.Score(), 0);
    }
    if constexpr (!feature || feature == 2)
    {
        d->ref.Main().TransformTo(jxl::ColorEncoding::LinearSRGB(false), jxl::GetJxlCms());
        d->dist.Main().TransformTo(jxl::ColorEncoding::LinearSRGB(false), jxl::GetJxlCms());

        ssimulacra::Ssimulacra ssimulacra_{ ssimulacra::ComputeDiff(*d->ref.Main().color(), *d->dist.Main().color(), d->simple) };
        avs_prop_set_float(fi->env, props, "SSIMULACRA", ssimulacra_.Score(), 0);
    }

    avs_release_video_frame(src2);

    return src1;
}

static void AVSC_CC free_ssimulacra(AVS_FilterInfo* fi)
{
    ssimulacra_data* d{ static_cast<ssimulacra_data*>(fi->user_data) };

    avs_release_clip(d->clip1);
    delete d;
}

static int AVSC_CC set_cache_hints_ssimulacra(AVS_FilterInfo* fi, int cachehints, int frame_range)
{
    return cachehints == AVS_CACHE_GET_MTMODE ? 2 : 0;
}

static AVS_Value AVSC_CC Create_ssimulacra(AVS_ScriptEnvironment* env, AVS_Value args, void* param)
{
    enum { Clip, Clip1, Simple, Feature };

    ssimulacra_data* d{ new ssimulacra_data() };

    AVS_FilterInfo* fi;
    AVS_Clip* clip{ avs_new_c_filter(env, &fi, avs_array_elt(args, Clip), 1) };

    const auto set_error{ [&](const char* error)
        {
            avs_release_clip(clip);

            return avs_new_value_error(error);
        }
    };

    if (!avs_check_version(env, 9))
    {
        if (avs_check_version(env, 10))
        {
            if (avs_get_env_property(env, AVS_AEP_INTERFACE_BUGFIX) < 2)
                return set_error("ssimulacra: AviSynth+ version must be r3688 or later.");
        }
    }
    else
        return set_error("ssimulacra: AviSynth+ version must be r3688 or later.");

    if (!avs_is_planar_rgb(&fi->vi))
        return set_error("ssimulacra: the clip must be in RGB planar format (w/o alpha).");
    if (avs_bits_per_component(&fi->vi) != 8 && avs_bits_per_component(&fi->vi) != 16 && avs_bits_per_component(&fi->vi) != 32)
        return set_error("ssimulacra: the clip bit depth must be 8, 16, or 32.");
    if (fi->vi.height < 8 || fi->vi.width < 8)
        return set_error("ssimulacra: minimum image size is 8x8 pixels");

    d->clip1 = avs_take_clip(avs_array_elt(args, Clip1), env);

    const AVS_VideoInfo* vi1{ avs_get_video_info(d->clip1) };

    if (!avs_is_same_colorspace(&fi->vi, vi1))
        return set_error("ssimulacra: the clips format doesn't match");
    if (vi1->num_frames != fi->vi.num_frames)
        return set_error("ssimulacra: the clips number of frames doesn't match");
    if ((vi1->width != fi->vi.width) || (vi1->height != fi->vi.height))
        return set_error("ssimulacra: the clips dimension doesn't match");

    d->simple = avs_defined(avs_array_elt(args, Simple)) ? avs_as_bool(avs_array_elt(args, Simple)) : 0;
    const int feature{ avs_defined(avs_array_elt(args, Feature)) ? avs_as_bool(avs_array_elt(args, Feature)) : 0 };

    if (feature < 0 || feature > 2)
        return set_error("ssimulacra: feature must be between 0..2.");

    switch (avs_component_size(&fi->vi))
    {
        case 1: d->fill = fill_image_ssimulacra<uint8_t>; break;
        case 2: d->fill = fill_image_ssimulacra<uint16_t>; break;
        default: d->fill = fill_image_ssimulacra<float>; break;
    }

    switch (feature)
    {
        case 0: fi->get_frame = get_frame_ssimulacra<0>; break;
        case 1: fi->get_frame = get_frame_ssimulacra<1>; break;
        default: fi->get_frame = get_frame_ssimulacra<2>; break;
    }

    d->ref.SetSize(fi->vi.width, fi->vi.height);
    d->dist.SetSize(fi->vi.width, fi->vi.height);

    AVS_Value v{ avs_new_value_clip(clip) };

    fi->user_data = reinterpret_cast<void*>(d);
    fi->set_cache_hints = set_cache_hints_ssimulacra;
    fi->free_filter = free_ssimulacra;

    avs_release_clip(clip);

    return v;
}

const char* AVSC_CC avisynth_c_plugin_init(AVS_ScriptEnvironment* env)
{
    avs_add_function(env, "ssimulacra", "cc[simple]b[feature]i", Create_ssimulacra, 0);
    return "ssimulacra";
}
