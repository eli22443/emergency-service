package bgu.spl.net.impl.stomp;

import bgu.spl.net.srv.Server;

public class StompServer {

    public static void main(String[] args) {
        if(args[1].equals("tpc")){
            Server.threadPerClient(
                    Integer.parseInt(args[0]), // port
                    FrameStompMessagingProtocol::new, // protocol factory
                    FrameMessageEncodeDecoder::new // message encoder decoder factory
            ).serve();}
        if(args[1].equals("reactor")){
            Server.reactor(
                    Runtime.getRuntime().availableProcessors(), // nthreads
                    Integer.parseInt(args[0]), // port
                    FrameStompMessagingProtocol::new, // protocol factory
                    FrameMessageEncodeDecoder::new // message encoder decoder factory
            ).serve();}
        else
            System.out.println("Invalid input");
    }
}
