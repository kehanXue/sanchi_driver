#include <utility>

#include <sanchi_driver/SanchiSerialHardware.h>
#include <sanchi_driver/utils.h>
#include <eigen3/Eigen/Geometry> 
#include <iostream>

using namespace vwpp;

SanchiSerialHardware::SanchiSerialHardware(std::string model_, std::string port_, int baud_, 
                                           int msg_length_,
                                           boost_serial_base::flow_control::type fc_type_,
                                           boost_serial_base::parity::type pa_type_, 
                                           boost_serial_base::stop_bits::type st_type_):
    model(std::move(model_)),
    port(std::move(port_)),
    baud(baud_),
    msg_length(msg_length_),
    fc_type(fc_type_),
    pa_type(pa_type_),
    st_type(st_type_)
{

    uint8_t tmp_msg_start[6] = {0xA5, 0x5A, 0x04, 0x01, 0x05, 0xAA};
    uint8_t tmp_msg_stop[6]  = {0xA5, 0x5A, 0x04, 0x02, 0x06, 0xAA};

    // TODO consider change to const
    this->msg_start = std::vector<uint8_t>(std::begin(tmp_msg_start), std::end(tmp_msg_start));
    this->msg_stop  = std::vector<uint8_t>(std::begin(tmp_msg_stop), std::end(tmp_msg_stop));

    this->boost_serial_communicator = new BoostSerialCommunicator(this->port, this->baud);

    if(model == "100D2")
    {
        boost_serial_communicator->sendMessage(msg_stop);
        usleep(1000 * 1000);
        boost_serial_communicator->sendMessage(msg_start);
        usleep(1000 * 1000);
    }

    std::cout << "\033[32m" << "Start streaming data..."
              << "\033[0m" << std::endl;

}

SanchiSerialHardware::~SanchiSerialHardware()
{
    delete this->boost_serial_communicator;
}

std::queue<SanchiSerialHardware::SanchiData> SanchiSerialHardware::readData()
{
    static uint8_t* data_raw;

    data_raw = boost_serial_communicator->getMessage(this->msg_length);

    int flag_msg_rev = 0;

    // When get the new data_raw, clean up the queue buf before.
    while ( !(this->que_sanchi_data.empty()) ) 
    {
        this->que_sanchi_data.pop();
    }

    const int msg_buf_length = 2*(this->msg_length);
    for (int header_index = 0; header_index < (msg_buf_length - 1); ++header_index)
    {
        if(model == "100D2" && data_raw[header_index] == 0xA5 && data_raw[header_index + 1] == 0x5A)
        {
            // When the data queue get errors.
            if (boost_serial_communicator->fixError(header_index, this->msg_length) == -1)
            {
                // continue;
                break;
            }


            unsigned char *data = data_raw + header_index;
            uint8_t data_length = data[2];


            uint32_t checksum = 0;
            for(int i = 0; i < data_length - 1  ; ++i)
            {
                checksum += (uint32_t) data[i+2];
            }

            uint16_t check = checksum % 256;
            uint16_t check_true = data[data_length+1];

            if (check != check_true)
            {
                std::cout << "check error" << std::endl;
                continue;
            }


            SanchiData per_sanchi_data;
            Eigen::Vector3d ea0(-d2f_euler(data + 3) * M_PI / 180.0,
                                 d2f_euler(data + 7) * M_PI / 180.0,
                                 d2f_euler(data + 5) * M_PI / 180.0);
            Eigen::Matrix3d R;
            R = Eigen::AngleAxisd(ea0[0], ::Eigen::Vector3d::UnitZ())
                * Eigen::AngleAxisd(ea0[1], ::Eigen::Vector3d::UnitY())
                * Eigen::AngleAxisd(ea0[2], ::Eigen::Vector3d::UnitX());
            Eigen::Quaterniond q;
            q = R;
            per_sanchi_data.q_.w = (double)q.w();
            per_sanchi_data.q_.x = (double)q.x();
            per_sanchi_data.q_.y = (double)q.y();
            per_sanchi_data.q_.z = (double)q.z();

            per_sanchi_data.av_.x = d2f_gyro(data + 15) * M_PI /180;
            per_sanchi_data.av_.y = d2f_gyro(data + 17) * M_PI /180;
            per_sanchi_data.av_.z = d2f_gyro(data + 19) * M_PI /180;

            per_sanchi_data.la_.x = d2f_acc(data + 9) * 9.81;
            per_sanchi_data.la_.y = d2f_acc(data + 11) * 9.81;
            per_sanchi_data.la_.z = d2f_acc(data + 13) * 9.81;

            per_sanchi_data.mf_.x = d2f_mag(data + 21);
            per_sanchi_data.mf_.y = d2f_mag(data + 23);
            per_sanchi_data.mf_.z = d2f_mag(data + 25);

            this->que_sanchi_data.push(per_sanchi_data);
        }

    }

    delete[] data_raw;

    return (this->que_sanchi_data);
}
