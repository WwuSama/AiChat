#ifndef __LLM_SERVICE_H_
#define __LLM_SERVICE_H_

void llm_init(void);

void llm_cleanup(void);

void llm_service(const char *user_content, char **answer);

void llm_free_answer(char *answer);

#endif