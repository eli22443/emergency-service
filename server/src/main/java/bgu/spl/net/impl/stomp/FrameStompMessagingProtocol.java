package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;

public class FrameStompMessagingProtocol implements StompMessagingProtocol<Frame> {

    // private int clientId;
    private boolean shouldTerminate = false;

    @Override
    public void start(int connectionId, Connections<Frame> connections) {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'start'");
    }

    @Override
    public void process(Frame message) {
        // TODO Auto-generated method stub
        throw new UnsupportedOperationException("Unimplemented method 'process'");
    }

    @Override
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
    
}
