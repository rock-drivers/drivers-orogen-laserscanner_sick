/* Generated from orogen/lib/orogen/templates/tasks/Task.cpp */

#include "Task.hpp"

using namespace laserscanner_sick;

Task::Task(std::string const& name)
    : TaskBase(name)
    , sick(0)
    , activity(0)
{
    status = 1;
    num_measurements = 0; 
    resolution = 0.0;
    start_angle = 0.0;
    timestamp_estimator = 0;
}

Task::Task(std::string const& name, RTT::ExecutionEngine* engine)
    : TaskBase(name, engine)
    , sick(0)
    , activity(0)
{
    status = 1;
    num_measurements = 0;
    resolution = 0.0;
    start_angle = 0.0;
    timestamp_estimator = 0;
}

Task::~Task()
{
    delete timestamp_estimator;
}

/// The following lines are template definitions for the various state machine
// hooks defined by Orocos::RTT. See Task.hpp for more detailed
// documentation about them.

bool Task::configureHook()
{
    if (! TaskBase::configureHook())
        return false;
    sick = new SickToolbox::SickLMS1xx(_hostname.get(), _port.get());
    timestamp_estimator = new aggregator::TimestampEstimator(base::Time::fromSeconds(2),base::Time::fromSeconds(0.02),INT_MAX);

    try{
	sick->Initialize();
    }
    catch(...){
	cerr << cerr << "Initialize failed! Are you using the correct IP address?" << endl;
	return false;
    }

    try{
	sick->SetSickScanDataFormat(SickToolbox::SickLMS1xx::SICK_LMS_1XX_SCAN_FORMAT_DIST_DOUBLE_PULSE_REFLECT_16BIT);
    }
    catch(SickToolbox::SickConfigException sick_exception){
	std::cout << sick_exception.what() << std::endl;
	return false;
    }

    if(sick->IntToSickScanFreq(_frequency.get()) == SickToolbox::SickLMS1xx::SICK_LMS_1XX_SCAN_FREQ_UNKNOWN)
	return false;
    
    if(sick->DoubleToSickScanRes(_resolution.get()) == SickToolbox::SickLMS1xx::SICK_LMS_1XX_SCAN_RES_UNKNOWN)
	return false;

    sick->SetSickScanFreqAndRes(sick->IntToSickScanFreq(_frequency.get()), sick->DoubleToSickScanRes(_resolution.get()));

    try{
	resolution = (sick->SickScanResToDouble(sick->GetSickScanRes()))/180.0*M_PI;
	//printf("Resolution: %f\n", sick->SickScanResToDouble(sick->GetSickScanRes()));
	start_angle = -(M_PI*0.5)+((sick->GetSickStartAngle())/180.0*M_PI);
	//printf("Frequency: %i\n", sick->SickScanFreqToInt(sick->GetSickScanFreq()));
    }
    catch(SickToolbox::SickIOException sick_exception) {
	std::cout << sick_exception.what() << std::endl;
	return false;
    }
    activity = getActivity<RTT::extras::FileDescriptorActivity>();

    return true;
}

bool Task::startHook()
{
    if (! TaskBase::startHook() || !activity)
	return false;
    activity->watch(sick->getReadFD());
    return true;
}

void Task::updateHook()
{
    TaskBase::updateHook();
    base::samples::LaserScan scan;
    _timestamp_estimator_status.write(timestamp_estimator->getStatus());

    try{
	sick->GetSickMeasurements(range_1_vals,range_2_vals,reflect_1_vals,reflect_2_vals,num_measurements,&status);
	for(int i=0; i<num_measurements; i++){
	    scan.ranges.push_back(range_1_vals[i]);
	    //std::cout << range_1_vals[i] << std::endl;
	}
	for(int i=0; i<num_measurements; i++){
	    scan.remission.push_back(reflect_1_vals[i]);
	}
    }
    catch(SickToolbox::SickIOException sick_exception) {
	std::cout << sick_exception.what() << std::endl;
	exception();
	return;
    }
    catch(SickToolbox::SickTimeoutException sick_exception) {
	std::cout << sick_exception.what() << std::endl;
	exception();
	return;
    }
    catch(...) {
	cerr << "An Error Occurred!" << endl;
	exception();
	return;
    }
    scan.time = timestamp_estimator->update(base::Time::now());
    scan.start_angle = start_angle;
    scan.angular_resolution = resolution;
    scan.speed = 50.0*(2.0*M_PI);
    scan.minRange = 500; 
    scan.maxRange = 20000;

    _io_status.write(sick->getBufferMonitorStatus());
    _scan.write(scan);
}

// void Task::errorHook()
// {
//     TaskBase::errorHook();
// }
// void Task::stopHook()
// {
//     TaskBase::stopHook();
// }

void Task::cleanupHook()
{
    TaskBase::cleanupHook();
    if(sick)
        delete sick;
    sick = 0;
}

