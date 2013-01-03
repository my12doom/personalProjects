#include <Windows.h>
#include "session.h"



class session_manager
{
public:
	session_manager();
	~session_manager();

	HRESULT add_one_frame();		// this will push a packet into the manager's packet queue, and remove oldest packets
									// and will push this packet into each active session.

	HRESULT new_session();			// the manager will push all queued packets into newly created session
									// the session is responsible for filtering and sending packets
protected:

	HRESULT unregister_session();	// just remove the session from manager's list, the session will not receive any more packets.
									// the session is responsible for necessary resource release.
};