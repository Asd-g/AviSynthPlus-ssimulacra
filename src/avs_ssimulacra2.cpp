#include "avs_ssimulacra.h"
#include "lib/extras/codec.h"
#include "lib/jxl/color_management.h"
#include "lib/jxl/enc_color_management.h"
#include "tools/ssimulacra2.h"

struct ssimulacra2_data
{
    AVS_Clip* clip1;
    jxl::CodecInOut ref;
    jxl::CodecInOut dist;
    int simple;

    void(*fill)(jxl::CodecInOut& __restrict ref, jxl::CodecInOut& __restrict dist, AVS_VideoFrame* src1, AVS_VideoFrame* src2) noexcept;
};

template <typename T>
static void fill_image_ssimulacra2(jxl::CodecInOut& __restrict ref, jxl::CodecInOut& __restrict dist, AVS_VideoFrame* src1, AVS_VideoFrame* src2) noexcept
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

        ref.SetFromImage(std::move(jxl::ConvertToFloat(tmp1)), ref.metadata.m.color_encoding);
        dist.SetFromImage(std::move(jxl::ConvertToFloat(tmp2)), dist.metadata.m.color_encoding);
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

        ref.SetFromImage(std::move(jxl::ConvertToFloat(tmp1)), ref.metadata.m.color_encoding);
        dist.SetFromImage(std::move(jxl::ConvertToFloat(tmp2)), dist.metadata.m.color_encoding);
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

        ref.SetFromImage(std::move(tmp1), ref.metadata.m.color_encoding);
        dist.SetFromImage(std::move(tmp2), dist.metadata.m.color_encoding);
    }
}

static AVS_VideoFrame* AVSC_CC get_frame_ssimulacra2(AVS_FilterInfo* fi, int n)
{
    ssimulacra2_data* d{ static_cast<ssimulacra2_data*>(fi->user_data) };

    AVS_VideoFrame* src1{ avs_get_frame(fi->child, n) };
    if (!src1)
        return nullptr;

    AVS_VideoFrame* src2{ avs_get_frame(d->clip1, n) };

    d->fill(d->ref, d->dist, src1, src2);

    Msssim msssim{ ComputeSSIMULACRA2(d->ref.Main(), d->dist.Main()) };

    avs_make_writable(fi->env, &src1);
    avs_prop_set_float(fi->env, avs_get_frame_props_rw(fi->env, src1), "_Framessimulacra2", msssim.Score(), 0);

    avs_release_video_frame(src2);

    return src1;
}

static void AVSC_CC free_ssimulacra2(AVS_FilterInfo* fi)
{
    ssimulacra2_data* d{ static_cast<ssimulacra2_data*>(fi->user_data) };

    avs_release_clip(d->clip1);
    delete d;
}

static int AVSC_CC set_cache_hints_ssimulacra2(AVS_FilterInfo* fi, int cachehints, int frame_range)
{
    return cachehints == AVS_CACHE_GET_MTMODE ? 2 : 0;
}

AVS_Value AVSC_CC Create_ssimulacra2(AVS_ScriptEnvironment* env, AVS_Value args, void* param)
{
    enum { Clip, Clip1, Simple, Linput };

    ssimulacra2_data* d{ new ssimulacra2_data() };

    AVS_FilterInfo* fi;
    AVS_Value v{ avs_void };

    AVS_Clip* clip{ avs_new_c_filter(env, &fi, avs_array_elt(args, Clip), 1) };

    if (!avs_is_planar_rgb(&fi->vi))
        v = avs_new_value_error("ssimulacra2: the clip must be in RGB planar format (w/o alpha).");

    if (!avs_defined(v))
    {
        if (fi->vi.height < 8 || fi->vi.width < 8)
            v = avs_new_value_error("ssimulacra2: minimum image size is 8x8 pixels");
    }
    if (!avs_defined(v))
    {
        d->clip1 = avs_take_clip(avs_array_elt(args, Clip1), env);

        const AVS_VideoInfo* vi1{ avs_get_video_info(d->clip1) };

        if (!avs_is_same_colorspace(&fi->vi, vi1))
            v = avs_new_value_error("ssimulacra2: the clips format doesn't match");

        if (!avs_defined(v))
        {
            if (vi1->num_frames != fi->vi.num_frames)
                v = avs_new_value_error("ssimulacra2: the clips number of frames doesn't match");

            if (!avs_defined(v))
            {
                if ((vi1->width != fi->vi.width) || (vi1->height != fi->vi.height))
                    v = avs_new_value_error("ssimulacra2: the clips dimension doesn't match");
            }
        }
    }
    if (!avs_defined(v))
    {
        d->simple = avs_defined(avs_array_elt(args, Simple)) ? avs_as_bool(avs_array_elt(args, Simple)) : 0;
        const int linput{ avs_defined(avs_array_elt(args, Linput)) ? avs_as_bool(avs_array_elt(args, Linput)) : 0 };

        switch (avs_component_size(&fi->vi))
        {
            case 1: d->fill = fill_image_ssimulacra2<uint8_t>; break;
            case 2: d->fill = fill_image_ssimulacra2<uint16_t>; break;
            default: d->fill = fill_image_ssimulacra2<float>; break;
        }

        d->ref.SetSize(fi->vi.width, fi->vi.height);
        d->dist.SetSize(fi->vi.width, fi->vi.height);

        d->ref.metadata.m.color_encoding = jxl::ColorEncoding::SRGB(false);
        d->dist.metadata.m.color_encoding = jxl::ColorEncoding::SRGB(false);
    }
    if (!avs_defined(v))
    {
        v = avs_new_value_clip(clip);

        fi->user_data = reinterpret_cast<void*>(d);
        fi->get_frame = get_frame_ssimulacra2;
        fi->set_cache_hints = set_cache_hints_ssimulacra2;
        fi->free_filter = free_ssimulacra2;
    }

    avs_release_clip(clip);

    return v;
}
