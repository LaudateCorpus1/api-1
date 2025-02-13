/* SPDX-License-Identifier: Apache-2.0 */
/**
 * Copyright (c) 2019 Samsung Electronics Co., Ltd. All Rights Reserved.
 *
 * @file ml-api-inference-internal.c
 * @date 19 October 2021
 * @brief ML-API Internal Utility Functions for inference implementations
 * @see	https://github.com/nnstreamer/api
 * @author MyungJoo Ham <myungjoo.ham@samsung.com>
 * @bug No known bugs except for NYI items
 */

#include <string.h>

#include <nnstreamer_internal.h>
#include <nnstreamer_log.h>
#include <nnstreamer_plugin_api.h>
#include <nnstreamer_plugin_api_filter.h>
#include <tensor_typedef.h>
#include "ml-api-inference-internal.h"
#include "ml-api-internal.h"

/**
 * @brief The name of sub-plugin for defined neural net frameworks.
 * @note The sub-plugin for Android is not declared (e.g., snap)
 */
static const char *ml_nnfw_subplugin_name[] = {
  [ML_NNFW_TYPE_ANY] = "any",   /* DO NOT use this name ('any') to get the sub-plugin */
  [ML_NNFW_TYPE_CUSTOM_FILTER] = "custom",
  [ML_NNFW_TYPE_TENSORFLOW_LITE] = "tensorflow-lite",
  [ML_NNFW_TYPE_TENSORFLOW] = "tensorflow",
  [ML_NNFW_TYPE_NNFW] = "nnfw",
  [ML_NNFW_TYPE_MVNC] = "movidius-ncsdk2",
  [ML_NNFW_TYPE_OPENVINO] = "openvino",
  [ML_NNFW_TYPE_VIVANTE] = "vivante",
  [ML_NNFW_TYPE_EDGE_TPU] = "edgetpu",
  [ML_NNFW_TYPE_ARMNN] = "armnn",
  [ML_NNFW_TYPE_SNPE] = "snpe",
  [ML_NNFW_TYPE_PYTORCH] = "pytorch",
  [ML_NNFW_TYPE_NNTR_INF] = "nntrainer",
  [ML_NNFW_TYPE_VD_AIFW] = "vd_aifw",
  [ML_NNFW_TYPE_TRIX_ENGINE] = "trix-engine",
  NULL
};

/**
 * @brief Internal function to get the sub-plugin name.
 */
const char *
_ml_get_nnfw_subplugin_name (ml_nnfw_type_e nnfw)
{
  /* check sub-plugin for android */
  if (nnfw == ML_NNFW_TYPE_SNAP)
    return "snap";

  return ml_nnfw_subplugin_name[nnfw];
}

/**
 * @brief Allocates a tensors information handle from gst info.
 */
int
_ml_tensors_info_create_from_gst (ml_tensors_info_h * ml_info,
    GstTensorsInfo * gst_info)
{
  int status;

  if (!ml_info || !gst_info)
    return ML_ERROR_INVALID_PARAMETER;

  status = ml_tensors_info_create (ml_info);
  if (status != ML_ERROR_NONE)
    return status;

  _ml_tensors_info_copy_from_gst (*ml_info, gst_info);
  return ML_ERROR_NONE;
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
void
_ml_tensors_info_copy_from_gst (ml_tensors_info_s * ml_info,
    const GstTensorsInfo * gst_info)
{
  guint i, j;
  guint max_dim;

  if (!ml_info || !gst_info)
    return;

  _ml_tensors_info_initialize (ml_info);
  max_dim = MIN (ML_TENSOR_RANK_LIMIT, NNS_TENSOR_RANK_LIMIT);

  ml_info->num_tensors = gst_info->num_tensors;

  for (i = 0; i < gst_info->num_tensors; i++) {
    /* Copy name string */
    if (gst_info->info[i].name) {
      ml_info->info[i].name = g_strdup (gst_info->info[i].name);
    }

    /* Set tensor type */
    switch (gst_info->info[i].type) {
      case _NNS_INT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT32;
        break;
      case _NNS_UINT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT32;
        break;
      case _NNS_INT16:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT16;
        break;
      case _NNS_UINT16:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT16;
        break;
      case _NNS_INT8:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT8;
        break;
      case _NNS_UINT8:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT8;
        break;
      case _NNS_FLOAT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_FLOAT64;
        break;
      case _NNS_FLOAT32:
        ml_info->info[i].type = ML_TENSOR_TYPE_FLOAT32;
        break;
      case _NNS_INT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_INT64;
        break;
      case _NNS_UINT64:
        ml_info->info[i].type = ML_TENSOR_TYPE_UINT64;
        break;
      default:
        ml_info->info[i].type = ML_TENSOR_TYPE_UNKNOWN;
        break;
    }

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      ml_info->info[i].dimension[j] = gst_info->info[i].dimension[j];
    }

    for (; j < ML_TENSOR_RANK_LIMIT; j++) {
      ml_info->info[i].dimension[j] = 1;
    }
  }
}

/**
 * @brief Copies tensor meta info from gst tensors info.
 * @bug Thread safety required. Check its internal users first!
 */
void
_ml_tensors_info_copy_from_ml (GstTensorsInfo * gst_info,
    const ml_tensors_info_s * ml_info)
{
  guint i, j;
  guint max_dim;

  if (!gst_info || !ml_info)
    return;

  G_LOCK_UNLESS_NOLOCK (*ml_info);

  gst_tensors_info_init (gst_info);
  max_dim = MIN (ML_TENSOR_RANK_LIMIT, NNS_TENSOR_RANK_LIMIT);

  gst_info->num_tensors = ml_info->num_tensors;

  for (i = 0; i < ml_info->num_tensors; i++) {
    /* Copy name string */
    if (ml_info->info[i].name) {
      gst_info->info[i].name = g_strdup (ml_info->info[i].name);
    }

    /* Set tensor type */
    switch (ml_info->info[i].type) {
      case ML_TENSOR_TYPE_INT32:
        gst_info->info[i].type = _NNS_INT32;
        break;
      case ML_TENSOR_TYPE_UINT32:
        gst_info->info[i].type = _NNS_UINT32;
        break;
      case ML_TENSOR_TYPE_INT16:
        gst_info->info[i].type = _NNS_INT16;
        break;
      case ML_TENSOR_TYPE_UINT16:
        gst_info->info[i].type = _NNS_UINT16;
        break;
      case ML_TENSOR_TYPE_INT8:
        gst_info->info[i].type = _NNS_INT8;
        break;
      case ML_TENSOR_TYPE_UINT8:
        gst_info->info[i].type = _NNS_UINT8;
        break;
      case ML_TENSOR_TYPE_FLOAT64:
        gst_info->info[i].type = _NNS_FLOAT64;
        break;
      case ML_TENSOR_TYPE_FLOAT32:
        gst_info->info[i].type = _NNS_FLOAT32;
        break;
      case ML_TENSOR_TYPE_INT64:
        gst_info->info[i].type = _NNS_INT64;
        break;
      case ML_TENSOR_TYPE_UINT64:
        gst_info->info[i].type = _NNS_UINT64;
        break;
      default:
        gst_info->info[i].type = _NNS_END;
        break;
    }

    /* Set dimension */
    for (j = 0; j < max_dim; j++) {
      gst_info->info[i].dimension[j] = ml_info->info[i].dimension[j];
    }

    for (; j < NNS_TENSOR_RANK_LIMIT; j++) {
      gst_info->info[i].dimension[j] = 1;
    }
  }
  G_UNLOCK_UNLESS_NOLOCK (*ml_info);
}

/**
 * @brief Initializes the GStreamer library. This is internal function.
 */
int
_ml_initialize_gstreamer (void)
{
  GError *err = NULL;

  if (!gst_init_check (NULL, NULL, &err)) {
    if (err) {
      _ml_loge ("GStreamer has the following error: %s", err->message);
      g_clear_error (&err);
    } else {
      _ml_loge ("Cannot initialize GStreamer. Unknown reason.");
    }

    return ML_ERROR_STREAMS_PIPE;
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal helper function to validate model files.
 */
static int
__ml_validate_model_file (const char *const *model,
    const unsigned int num_models, gboolean * is_dir)
{
  guint i;

  if (!model || num_models < 1) {
    _ml_loge ("The required param, model is not provided (null).");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (g_file_test (model[0], G_FILE_TEST_IS_DIR)) {
    *is_dir = TRUE;
    return ML_ERROR_NONE;
  }

  for (i = 0; i < num_models; i++) {
    if (!model[i] || !g_file_test (model[i], G_FILE_TEST_IS_REGULAR)) {
      _ml_loge ("The given param, model path [%s] is invalid or not given.",
          GST_STR_NULL (model[i]));
      return ML_ERROR_INVALID_PARAMETER;
    }
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Internal function to get the nnfw type.
 */
ml_nnfw_type_e
_ml_get_nnfw_type_by_subplugin_name (const char *name)
{
  ml_nnfw_type_e nnfw_type = ML_NNFW_TYPE_ANY;
  int idx = -1;

  if (name == NULL)
    return ML_NNFW_TYPE_ANY;

  idx = find_key_strv (ml_nnfw_subplugin_name, name);
  if (idx < 0) {
    /* check sub-plugin for android */
    if (g_ascii_strcasecmp (name, "snap") == 0)
      nnfw_type = ML_NNFW_TYPE_SNAP;
    else
      _ml_logw ("Cannot find nnfw, %s is invalid name.", GST_STR_NULL (name));
  } else {
    nnfw_type = (ml_nnfw_type_e) idx;
  }

  return nnfw_type;
}

/**
 * @brief Validates the nnfw model file.
 * @since_tizen 5.5
 * @param[in] model The path of model file.
 * @param[in/out] nnfw The type of NNFW.
 * @return @c 0 on success. Otherwise a negative error value.
 * @retval #ML_ERROR_NONE Successful
 * @retval #ML_ERROR_NOT_SUPPORTED Not supported, or framework to support this model file is unavailable in the environment.
 * @retval #ML_ERROR_INVALID_PARAMETER Given parameter is invalid.
 */
int
_ml_validate_model_file (const char *const *model,
    const unsigned int num_models, ml_nnfw_type_e * nnfw)
{
  int status = ML_ERROR_NONE;
  ml_nnfw_type_e detected = ML_NNFW_TYPE_ANY;
  gboolean is_dir = FALSE;
  gchar *pos, *fw_name;
  gchar **file_ext = NULL;
  guint i;

  if (!nnfw)
    return ML_ERROR_INVALID_PARAMETER;

  status = __ml_validate_model_file (model, num_models, &is_dir);
  if (status != ML_ERROR_NONE)
    return status;

  /**
   * @note detect-fw checks the file ext and returns proper fw name for given models.
   * If detected fw and given nnfw are same, we don't need to check the file extension.
   * If any condition for auto detection is added later, below code also should be updated.
   */
  fw_name = gst_tensor_filter_detect_framework (model, num_models, TRUE);
  detected = _ml_get_nnfw_type_by_subplugin_name (fw_name);
  g_free (fw_name);

  if (*nnfw == ML_NNFW_TYPE_ANY) {
    if (detected == ML_NNFW_TYPE_ANY) {
      _ml_loge ("The given model has unknown or not supported extension.");
      status = ML_ERROR_INVALID_PARAMETER;
    } else {
      _ml_logi ("The given model is supposed a %s model.",
          _ml_get_nnfw_subplugin_name (detected));
      *nnfw = detected;
    }

    goto done;
  } else if (is_dir && *nnfw != ML_NNFW_TYPE_NNFW) {
    /* supposed it is ONE if given model is directory */
    _ml_loge ("The given model is directory, check model and framework.");
    status = ML_ERROR_INVALID_PARAMETER;
    goto done;
  } else if (detected == *nnfw) {
    /* Expected framework, nothing to do. */
    goto done;
  }

  /* Handle mismatched case, check file extension. */
  file_ext = g_malloc0 (sizeof (char *) * (num_models + 1));
  for (i = 0; i < num_models; i++) {
    if ((pos = strrchr (model[i], '.')) == NULL) {
      _ml_loge ("The given model [%s] has invalid extension.", model[i]);
      status = ML_ERROR_INVALID_PARAMETER;
      goto done;
    }

    file_ext[i] = g_ascii_strdown (pos, -1);
  }

  /** @todo Make sure num_models is correct for each nnfw type */
  switch (*nnfw) {
    case ML_NNFW_TYPE_NNFW:
      /**
       * We cannot check the file ext with NNFW.
       * NNFW itself will validate metadata and model file.
       */
      break;
    case ML_NNFW_TYPE_MVNC:
    case ML_NNFW_TYPE_OPENVINO:
    case ML_NNFW_TYPE_EDGE_TPU:
      /** @todo Need to check method to validate model */
      _ml_loge ("Given NNFW is not supported yet.");
      status = ML_ERROR_NOT_SUPPORTED;
      break;
    case ML_NNFW_TYPE_VD_AIFW:
      if (!g_str_equal (file_ext[0], ".nb") &&
          !g_str_equal (file_ext[0], ".ncp") &&
          !g_str_equal (file_ext[0], ".bin")) {
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    case ML_NNFW_TYPE_SNAP:
#if !defined (__ANDROID__)
      _ml_loge ("SNAP only can be included in Android (arm64-v8a only).");
      status = ML_ERROR_NOT_SUPPORTED;
#endif
      /* SNAP requires multiple files, set supported if model file exists. */
      break;
    case ML_NNFW_TYPE_ARMNN:
      if (!g_str_equal (file_ext[0], ".caffemodel") &&
          !g_str_equal (file_ext[0], ".tflite") &&
          !g_str_equal (file_ext[0], ".pb") &&
          !g_str_equal (file_ext[0], ".prototxt")) {
        status = ML_ERROR_INVALID_PARAMETER;
      }
      break;
    default:
      status = ML_ERROR_INVALID_PARAMETER;
      break;
  }

done:
  if (status == ML_ERROR_NONE) {
    if (!_ml_nnfw_is_available (*nnfw, ML_NNFW_HW_ANY)) {
      _ml_loge ("%s is not available.", _ml_get_nnfw_subplugin_name (*nnfw));
      status = ML_ERROR_NOT_SUPPORTED;
    }
  } else {
    _ml_loge ("The given model file is invalid.");
  }

  g_strfreev (file_ext);
  return status;
}

/**
 * @brief Convert c-api based hw to internal representation
 */
accl_hw
_ml_nnfw_to_accl_hw (const ml_nnfw_hw_e hw)
{
  switch (hw) {
    case ML_NNFW_HW_ANY:
      return ACCL_DEFAULT;
    case ML_NNFW_HW_AUTO:
      return ACCL_AUTO;
    case ML_NNFW_HW_CPU:
      return ACCL_CPU;
#if defined (__aarch64__) || defined (__arm__)
    case ML_NNFW_HW_CPU_NEON:
      return ACCL_CPU_NEON;
#else
    case ML_NNFW_HW_CPU_SIMD:
      return ACCL_CPU_SIMD;
#endif
    case ML_NNFW_HW_GPU:
      return ACCL_GPU;
    case ML_NNFW_HW_NPU:
      return ACCL_NPU;
    case ML_NNFW_HW_NPU_MOVIDIUS:
      return ACCL_NPU_MOVIDIUS;
    case ML_NNFW_HW_NPU_EDGE_TPU:
      return ACCL_NPU_EDGE_TPU;
    case ML_NNFW_HW_NPU_VIVANTE:
      return ACCL_NPU_VIVANTE;
    case ML_NNFW_HW_NPU_SLSI:
      return ACCL_NPU_SLSI;
    case ML_NNFW_HW_NPU_SR:
      /** @todo how to get srcn npu */
      return ACCL_NPU_SR;
    default:
      return ACCL_AUTO;
  }
}

/**
 * @brief Internal function to convert accelerator as tensor_filter property format.
 * @note returned value must be freed by the caller
 * @note More details on format can be found in gst_tensor_filter_install_properties() in tensor_filter_common.c.
 */
char *
_ml_nnfw_to_str_prop (const ml_nnfw_hw_e hw)
{
  const gchar *hw_name;
  const gchar *use_accl = "true:";
  gchar *str_prop = NULL;

  hw_name = get_accl_hw_str (_ml_nnfw_to_accl_hw (hw));
  str_prop = g_strdup_printf ("%s%s", use_accl, hw_name);

  return str_prop;
}

/**
 * @brief Checks the element is registered and available on the pipeline.
 */
int
ml_check_element_availability (const char *element_name, bool *available)
{
  GstElementFactory *factory;
  int status;

  check_feature_state ();

  if (!element_name || !available)
    return ML_ERROR_INVALID_PARAMETER;

  status = _ml_initialize_gstreamer ();
  if (status != ML_ERROR_NONE)
    return status;

  /* init false */
  *available = false;

  factory = gst_element_factory_find (element_name);
  if (factory) {
    GstPluginFeature *feature = GST_PLUGIN_FEATURE (factory);
    const gchar *plugin_name = gst_plugin_feature_get_plugin_name (feature);

    /* check restricted element */
    status = _ml_check_plugin_availability (plugin_name, element_name);
    if (status == ML_ERROR_NONE)
      *available = true;

    gst_object_unref (factory);
  }

  return ML_ERROR_NONE;
}

/**
 * @brief Checks the availability of the plugin.
 */
int
_ml_check_plugin_availability (const char *plugin_name,
    const char *element_name)
{
  static gboolean list_loaded = FALSE;
  static gchar **restricted_elements = NULL;

  if (!plugin_name || !element_name) {
    _ml_loge ("The name is invalid, failed to check the availability.");
    return ML_ERROR_INVALID_PARAMETER;
  }

  if (!list_loaded) {
    gboolean restricted;

    restricted =
        nnsconf_get_custom_value_bool ("element-restriction",
        "enable_element_restriction", FALSE);
    if (restricted) {
      gchar *elements;

      /* check white-list of available plugins */
      elements =
          nnsconf_get_custom_value_string ("element-restriction",
          "restricted_elements");
      if (elements) {
        restricted_elements = g_strsplit_set (elements, " ,;", -1);
        g_free (elements);
      }
    }

    list_loaded = TRUE;
  }

  /* nnstreamer elements */
  if (g_str_has_prefix (plugin_name, "nnstreamer") &&
      g_str_has_prefix (element_name, "tensor_")) {
    return ML_ERROR_NONE;
  }

  if (restricted_elements &&
      find_key_strv ((const gchar **) restricted_elements, element_name) < 0) {
    _ml_logw ("The element %s is restricted.", element_name);
    return ML_ERROR_NOT_SUPPORTED;
  }

  return ML_ERROR_NONE;
}
