%module milkcat_raw
%{
typedef struct milkcat_t milkcat_t;

milkcat_t *milkcat_init(int processor_type, const char *model_dir_path);
void milkcat_delete(milkcat_t *processor);
void milkcat_process(milkcat_t *processor, const char *text);
int milkcat_next(milkcat_t *processor);
const char *milkcat_get_word(milkcat_t *processor);
const char *milkcat_get_part_of_speech_tag(milkcat_t *processor);
int milkcat_get_word_type(milkcat_t *processor);
const char *milkcat_get_error_message();
%}

milkcat_t *milkcat_init(int processor_type, const char *model_dir_path);
void milkcat_delete(milkcat_t *processor);
void milkcat_process(milkcat_t *processor, const char *text);
int milkcat_next(milkcat_t *processor);
const char *milkcat_get_word(milkcat_t *processor);
const char *milkcat_get_part_of_speech_tag(milkcat_t *processor);
int milkcat_get_word_type(milkcat_t *processor);
const char *milkcat_get_error_message();