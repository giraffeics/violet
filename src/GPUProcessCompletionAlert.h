#include "GPUProcess.h"

/**
 *  Takes one passable resource as a dependency, alerts the console
 *  when that passable resource's semaphore is signaled.
 *  This is very messy, quick-and-dirty code, and should be only
 *  be used when you need to confirm that a given process is actually
 *  completing execution; unless this code gets cleaned up, the class should
 *  NOT be referenced in any committed code.
 */
class GPUProcessCompletionAlert : public GPUProcess
{
public:
    // constructors and destructor
    ~GPUProcessCompletionAlert();

    // public functionality
    void setPR(const PassableResourceBase* pr);

    // virtual functions inherited from GPUProcess
    virtual OperationType getOperationType() { return OP_TYPE_OTHER; }
    virtual void acquireLongtermResources();
    virtual std::vector<PRDependency> getPRDependencies();
    virtual bool performOperation(std::vector<VkSemaphore> waitSemaphores, VkFence fence, VkSemaphore semaphore);

private:
    VkFence mFence = VK_NULL_HANDLE;
    VkBuffer mBuffer = VK_NULL_HANDLE;
    VkDeviceMemory mBufferMemory = VK_NULL_HANDLE;
    const PassableResourceBase* mPassableResource;

};