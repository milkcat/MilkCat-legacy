import milkcat_raw

NORMAL_PROCESSOR = 0
CRF_SEGMENTER = 1
CRF_PROCESSOR = 2
HMM_PROCESSOR = 3

class MilkCat:
    def __init__(self, model_path, processor_type = NORMAL_PROCESSOR):
        self._processor_type = processor_type
        self._milkcat_obj = milkcat_raw.milkcat_init(processor_type, model_path)

    def _get_item(self):
        if self._processor_type == CRF_SEGMENTER:
            return milkcat_raw.milkcat_get_word(self._milkcat_obj)
        else:
            return (milkcat_raw.milkcat_get_word(self._milkcat_obj),
                    milkcat_raw.milkcat_get_part_of_speech_tag(self._milkcat_obj))

    def process(self, text):
        result = []
        milkcat_raw.milkcat_process(self._milkcat_obj, text)
        while milkcat_raw.milkcat_next(self._milkcat_obj) == 1:
            result.append(self._get_item())
        return result