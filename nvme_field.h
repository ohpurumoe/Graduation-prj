#include "nvme_queue_entry.h"

void set_PSDT(uchar PSDT, struct iosq_entry *element);
void set_FUSE(uchar FUSE, struct iosq_entry *element);
void set_StatusField(uchar Status_Field[2], struct iocq_entry element);
void set_P(uchar p, struct iocq_entry element);
uchar get_PSDT(struct iosq_entry element);
uchar get_FUSE(struct iosq_entry element);
uint get_StatusField(struct iocq_entry element);
uchar get_P(struct iocq_entry element);

/*
below is aboue admin setting

identify
*/
void admin_identify_set_controller_identifier(uint controller_identifier,struct iosq_entry *element);
void admin_identify_set_CNS(uchar CNS,struct iosq_entry *element);

/*
create io completion queue function
*/
void admin_create_io_completion_queue_set_queue_size(uint queue_size,struct iosq_entry *element);
void admin_create_io_completion_queue_set_queue_identfier(uint queue_identifier,struct iosq_entry *element);
void admin_create_io_completion_queue_set_interrupt_vector(uint interrupt_vector,struct iosq_entry *element);
void admin_create_io_completion_queue_set_ien(uchar ien, struct iosq_entry *element);
void admin_create_io_completion_queue_set_pc(uchar pc,struct iosq_entry *element);


/*
create io submission queue function

*/
void admin_create_io_submission_queue_set_queue_size(uint queue_size,struct iosq_entry *element);
void admin_create_io_submission_queue_set_queue_identfier(uint queue_identifier,struct iosq_entry *element);
void admin_create_io_submission_queue_set_completion_queue_identifier(uint completion_queue_identifier,struct iosq_entry *element);
void admin_create_io_submission_queue_set_QPRIO(uchar QPRIO, struct iosq_entry *element);
void admin_create_io_submission_queue_set_pc(uchar pc, struct iosq_entry *element);
/*
set feature
*/

void admin_set_feature_set_sv(uchar sv, struct iosq_entry *element);
void admin_set_feature_set_feature_identfier(uchar feature_identifier, struct iosq_entry *element);
void admin_set_feature_feature_specific(uint feature_specific[5], struct iosq_entry *element);

/*
read/write command
*/


void rw_command_set_lba(uint starting_lba[2], struct iosq_entry *element);
void rw_command_set_num_lb(uint num_lb, struct iosq_entry *element);