import React from 'react';

const Video: React.FC = () => {
    return (
        <div>
            <iframe
                src="http://192.168.1.100:8081"
                width="600"
                height="400"
                allow="autoplay"
                frameBorder="0"
            ></iframe>
        </div>
    );
};

export default Video;
