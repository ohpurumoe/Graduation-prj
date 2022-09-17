#include "nvme_field.h"
#include "defs.h"
void 
set_PSDT(uchar PSDT, struct iosq_entry *element)
{
    (*element).PSDT_FUSE &= 0x00ffffff;
    (*element).PSDT_FUSE |= (PSDT << 6);
}

void 
set_FUSE(uchar FUSE, struct iosq_entry *element)
{
    (*element).PSDT_FUSE = ((*element).PSDT_FUSE >> 2) << 2;
    (*element).PSDT_FUSE |= FUSE;
}

void 
set_StatusField(uchar Status_Field[2], struct iocq_entry element)
{
    element.status_field_p[1] = Status_Field[1];
    element.status_field_p[0] = Status_Field[0];
    element.status_field_p[1] = (element.status_field_p[1] >> 1) << 1;
}

void 
set_P(uchar p, struct iocq_entry element)
{
    element.status_field_p[0] |= p; 
}

uchar 
get_PSDT(struct iosq_entry element)
{
    return element.PSDT_FUSE>>6;
}

uchar 
get_FUSE(struct iosq_entry element)
{
    return element.PSDT_FUSE&0b11;
}

uint 
get_StatusField(struct iocq_entry element)
{
    return ((element.status_field_p[1] << 8) + element.status_field_p[0]) >> 1;
}

uchar 
get_P(struct iocq_entry element)
{
    return element.status_field_p[0]&0b1;
}
/*
below is aboue admin setting

identify
*/
void
admin_identify_set_controller_identifier(uint controller_identifier,struct iosq_entry *element)
{
    //cprintf("func addr : %x\n",element);
    //cprintf("command_specific %x\n", (*element).command_specific[0]);
    (*element).command_specific[0] &= 0x0000ff00;
    (*element).command_specific[0] |= (controller_identifier << 16);

}

void
admin_identify_set_CNS(uchar CNS,struct iosq_entry *element)
{
   // (*element).command_specific[0] = ((*element).command_specific[0] >> 8) << 8;
    (*element).command_specific[0] &= 0xffffff00;
    (*element).command_specific[0] |= CNS;
}
/*
create io completion queue function
*/
void
admin_create_io_completion_queue_set_queue_size(uint queue_size,struct iosq_entry *element)
{
    (*element).command_specific[0] = ((*element).command_specific[0] << 16) >> 16;
    (*element).command_specific[0] |= (queue_size << 16);
}

void
admin_create_io_completion_queue_set_queue_identfier(uint queue_identifier,struct iosq_entry *element)
{
    (*element).command_specific[0] = ((*element).command_specific[0] >> 16) << 16;
    (*element).command_specific[0] |= queue_identifier;
}

void
admin_create_io_completion_queue_set_interrupt_vector(uint interrupt_vector,struct iosq_entry *element){
    (*element).command_specific[1] = ((*element).command_specific[1] << 16) >> 16;
    (*element).command_specific[1] |= (interrupt_vector << 16);
}


void
admin_create_io_completion_queue_set_ien(uchar ien, struct iosq_entry *element){
    (*element).command_specific[1] &= 0xfffffffd;
    (*element).command_specific[1] |= (ien << 1);
}

void
admin_create_io_completion_queue_set_pc(uchar pc,struct iosq_entry *element){
    (*element).command_specific[1] &= 0xfffffffe;
    (*element).command_specific[1] |= pc;
}


/*
create io submission queue function

*/
void
admin_create_io_submission_queue_set_queue_size(uint queue_size,struct iosq_entry *element)
{
    (*element).command_specific[0] = ((*element).command_specific[0] << 16) >> 16;
    (*element).command_specific[0] |= (queue_size << 16);
}

void
admin_create_io_submission_queue_set_queue_identfier(uint queue_identifier,struct iosq_entry *element)
{
    (*element).command_specific[0] = ((*element).command_specific[0] >> 16) << 16;
    (*element).command_specific[0] |= queue_identifier;
}

void
admin_create_io_submission_queue_set_completion_queue_identifier(uint completion_queue_identifier,struct iosq_entry *element){
    (*element).command_specific[1] = ((*element).command_specific[1] << 16) >> 16;
    (*element).command_specific[1] |= (completion_queue_identifier << 16);
}

void
admin_create_io_submission_queue_set_QPRIO(uchar QPRIO, struct iosq_entry *element){
    (*element).command_specific[1] &= 0xfffffff9;
    (*element).command_specific[1] |= (QPRIO << 1);
}

void
admin_create_io_submission_queue_set_pc(uchar pc, struct iosq_entry *element){
    (*element).command_specific[1] &= 0xfffffffe;
    (*element).command_specific[1] |= pc;
}
/*
set feature
*/

void
admin_set_feature_set_sv(uchar sv, struct iosq_entry *element){
    (*element).command_specific[0] = ((*element).command_specific[0] << 1) >> 1;
    (*element).command_specific[0] |= sv << 31; 
}

void 
admin_set_feature_set_feature_identfier(uchar feature_identifier, struct iosq_entry *element){
    (*element).command_specific[0] = ((*element).command_specific[0] >> 8) << 8;
    (*element).command_specific[0] |= feature_identifier;
}

void
admin_set_feature_feature_specific(uint feature_specific[5], struct iosq_entry *element){
    for (int i = 1; i < 5; i++){
        (*element).command_specific[i] = feature_specific[i-1];
    }
}

/*
read/write command
*/


void
rw_command_set_lba(uint starting_lba[2], struct iosq_entry *element){
    //cprintf("startlba%x %x\n",starting_lba[1],starting_lba[0] );
    for (int i = 0; i < 2; i++){
        (*element).command_specific[i]=starting_lba[i];
    }
}

void
rw_command_set_num_lb(uint num_lb, struct iosq_entry *element){
    (*element).command_specific[2] = ((*element).command_specific[2] >> 16) << 16;
    (*element).command_specific[2] |= num_lb;
}